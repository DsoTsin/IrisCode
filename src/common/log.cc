#include "log.hpp"
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <vector>
#if _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace iris {
std::atomic_bool is_initialized{false};
bool is_console = false;
#if _WIN32
class console {
public:
  console() {
    stdout_ = ::GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    is_console_ = !!::GetConsoleScreenBufferInfo(stdout_, &info);
    attrib_ = info.wAttributes;
    const char* term = ::getenv("TERM");
    if (term && std::string(term) == "dumb") {
      smart_term_ = false;
    } else {
      setvbuf(stdout, NULL, _IONBF, 0);
      CONSOLE_SCREEN_BUFFER_INFO csbi;
      smart_term_ = GetConsoleScreenBufferInfo(stdout_, &csbi);
    }
    bool supports_color_ = smart_term_;
    if (!supports_color_) {
      const char* clicolor_force = getenv("CLICOLOR_FORCE");
      supports_color_ = clicolor_force && std::string(clicolor_force) != "0";
    }
    if (supports_color_) {
      DWORD mode;
      if (GetConsoleMode(stdout_, &mode)) {
        if (!SetConsoleMode(stdout_, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
          supports_color_ = false;
        }
      }
    }
  }
  ~console() {}

  void write(const std::string& msg) {
    DWORD written = 0;
    printf("%s", msg.c_str());
    //::WriteConsoleA(stdout_, msg.c_str(), msg.length(), &written, NULL);
    //::FlushFileBuffers(stdout_);
  }
  bool is_console() const { return is_console_; }

  void restore_attrib() { ::SetConsoleTextAttribute(stdout_, (WORD)attrib_); }

  void set_text_attrib(DWORD attrib) { ::SetConsoleTextAttribute(stdout_, attrib); }

private:
  HANDLE stdout_;
  DWORD attrib_;
  bool smart_term_;
  bool is_console_;
};
std::shared_ptr<console> con_;
#endif

void ensure_initialized() {
  if (!is_initialized.load()) {
#if _WIN32
    con_ = std::make_shared<console>();
    is_console = con_->is_console();
#else
    is_console = isatty(fileno(stdout));
#endif
    is_initialized.store(true);
  }
}

std::mutex s_logmutex;

struct _logger {
  log_fn fn = nullptr;
  void* d = nullptr;
};

static std::vector<_logger> loggers;

void install_logger(log_fn fn, void* data) {
  std::lock_guard<std::mutex> lock(s_logmutex);
  loggers.push_back({fn, data});
}

void log(log_level lv, bool asline, int line, const char* file, const char* fmt, ...) {
  std::lock_guard<std::mutex> lock(s_logmutex);
  ensure_initialized();
  static char log_buf[2048];
  va_list args;
  va_start(args, fmt);
  int length = vsnprintf(log_buf, 2048, fmt, args);
  std::string log_str;
  log_str.reserve(length + 1);
  log_str.resize(length);
  vsnprintf(log_str.data(), length + 1, fmt, args);
  va_end(args);
  switch (lv) {
  case log_warning:
    config_console_color(cc_yellow);
    break;
  case log_error:
  case log_fatal:
    config_console_color(cc_red);
    break;
  case log_info:
  default:
    config_console_color(cc_default);
    break;
  }
  if (asline) {
    log_str += "\n";
  }
#if _WIN32
  con_->write(log_str);
  OutputDebugStringA(log_str.c_str());
#else
  std::cout << log_str;
#endif
  for (auto& logger : loggers) {
    logger.fn(lv, log_str.c_str(), logger.d);
  }

  config_console_color(cc_default);
}

void log_raw(log_level lv, const char* _log) {
  std::lock_guard<std::mutex> lock(s_logmutex);
  ensure_initialized();
#if _WIN32
  con_->write(_log);
  OutputDebugStringA(_log);
#else
  std::cout << _log;
#endif
  for (auto& logger : loggers) {
    logger.fn(lv, _log, logger.d);
  }
}

void log(const char* fmt, ...) {
  std::lock_guard<std::mutex> lock(s_logmutex);
  ensure_initialized();
  static char log_buf1[2048];
  va_list args;
  va_start(args, fmt);
  vsnprintf(log_buf1, 2048, fmt, args);
  va_end(args);
  std::cout << log_buf1;
}

void config_console_color(console_color c) {
  ensure_initialized();
  switch (c) {
  case cc_default:
#if _WIN32
    con_->restore_attrib();
#endif
    break;
  case cc_red:
#if _WIN32
    con_->set_text_attrib(FOREGROUND_RED | FOREGROUND_INTENSITY);
#endif
    break;
  case cc_yellow:
#if _WIN32
    con_->set_text_attrib(FOREGROUND_RED | FOREGROUND_GREEN);
#endif
    break;
  case cc_green:
#if _WIN32
    con_->set_text_attrib(FOREGROUND_GREEN);
#endif
    break;
  case cc_blue:
#if _WIN32
    con_->set_text_attrib(FOREGROUND_BLUE | FOREGROUND_INTENSITY);
#endif
    break;
  default:
    break;
  }
}
} // namespace iris
