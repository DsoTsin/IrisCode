#pragma once

#include "stl.hpp"

namespace iris {
extern string uuid(const string& name);
extern int get_num_cores();
extern uint64_t get_total_physical_memory();
extern uint64_t get_available_physical_memory();
extern int64_t get_cpu_frequency();
extern float get_cpu_overall_usage();
extern void get_sys_mem_info(int64_t& physical_total, int64_t& physical_avail);
extern bool execute_command_line(string const& command_line,
                                 string const& startup_dir,
                                 string& _stdout,
                                 string& _stderr,
                                 int& ret_code);

extern bool start_command_line_as_su(string const& command_line, string const& startup_dir);

extern void list_dirs(string const& dir, vector<string>& out_dirs);
extern bool exists(string const& file);
/// <summary>
/// Get profile directory for current user
/// </summary>
/// <returns></returns>
extern string get_user_dir();
extern bool is_an_admin();
extern bool launched_from_console();
extern int get_console_width();
extern string get_compute_name();
extern string_list env_pathes();
extern float get_dpi_at_point(int x, int y);
class path {
public:
  static bool exists(string const& path);
  static string file_name(string const& file_path);
  static string file_dir(string const& file_path);
  static string file_content_md5(string const& file_path);
  static string file_string(string const& file_path);
  static string file_basename(string const& file_path);
  static bool file_copy(const string& src, const string& dest);
  static bool file_remove(const string& file);
  static bool is_absolute(string const& path);
  static string get_user_dir();
  static string_list list_dirs(string const& dir);
  /*
   * If has patterns, only supports **,*,*.ext
   * for examples, "/users/aopp\**.hpp" means listing
   * all files with ext hpp under dir "/users/aopp/"
   */
  static string_list list_files(string const& dir, bool has_pattern);
  static string_list list_files_by_ext(string const& dir, string const& ext, bool recurse = false);
  static string relative_to(string const& dir, string const& dest_dir);
  static bool make(string const& dir);
  static string join(string const& a, string const& b, string const& c = "");
  static string current_executable();
  static string current_dir(); // current working dir
  static string program_dir();
  static uint64_t get_file_timestamp(string const& file);
};

class sub_process_impl;

class sub_process {
public:
  class env {
    class sub_process_impl;

  public:
    explicit env(bool inherit_from_parent = true);
    explicit env(const wchar_t* strs);
    ~env();
    void* data() const;
    size_t size() const;
    void update(string const& key, string const& val);

  private:
    void* m_env_handle;
  };

  enum status {
    exit_success,
    exit_failure,
    exit_interrupted,
  };

  virtual ~sub_process();
  status finish();
  int exit_code() const;
  bool done() const;
  void* data() const;

  template <typename T>
  T* d() const {
    return reinterpret_cast<T*>(data());
  };

  const string& get_output();
  const string& get_cmd() const;

protected:
  sub_process(bool is_console, void* usr_data);
  bool start(struct sub_process_set* set,
             const string& cmd,
             string_list const& hookdlls,
             const env*);
  void on_pipe_ready();

  friend struct sub_process_set;
  friend class sub_process_impl;
  friend struct sub_process_set_impl;

  unique_ptr<sub_process_impl> m_d;
};

template <typename T>
class sub_process_obj : public sub_process {
public:
  ~sub_process_obj() {}

private:
  friend struct sub_process_set;
  sub_process_obj(const T& obj, bool is_console) : sub_process(is_console, obj.get()), m_obj(obj) {}
  const T& m_obj;
};

struct sub_process_set_impl;
struct sub_process_set {
  sub_process_set();
  ~sub_process_set();

  sub_process* add(string const& cmdline,
                   bool is_console = true,
                   void* usr_data = nullptr,
                   string_list const& hookdlls = string_list(),
                   const sub_process::env* penv = nullptr);

  template <typename T>
  sub_process* add(string_list const& cmd_args,
                   const T& obj,
                   bool is_console = true,
                   string_list const& hookdlls = string_list(),
                   const sub_process::env* penv = nullptr) {
    return add<T>(cmd_args.join(), obj, is_console, hookdlls, penv);
  }

  template <typename T>
  sub_process* add(string const& cmdline,
                   const T& obj,
                   bool is_console = true,
                   string_list const& hookdlls = string_list(),
                   const sub_process::env* penv = nullptr);

  sub_process* add_raw(string const& cmd_line,
                       void* raw_ptr,
                       bool is_console = true,
                       string_list const& hookdlls = string_list(),
                       const sub_process::env* penv = nullptr);

  bool do_work();
  sub_process* next_finished();
  bool has_running_procs() const;
  int num_running_procs() const;
  void clear();

  void set_limit(int num_procs);

  friend class sub_process_impl;

private:
  void on_new_subprocess(sub_process* subproc);
  unique_ptr<sub_process_set_impl> m_d;
};
template <typename T>
inline sub_process* sub_process_set::add(string const& cmdline,
                                         const T& obj,
                                         bool is_console,
                                         string_list const& hookdlls,
                                         const sub_process::env* penv) {
  sub_process* subprocess = new sub_process_obj<T>(obj, is_console);
  if (!subprocess->start(this, cmdline, hookdlls, penv)) {
    delete subprocess;
    return 0;
  }
  on_new_subprocess(subprocess);
  return subprocess;
}

inline sub_process* sub_process_set::add_raw(string const& cmd_line,
                                             void* raw_ptr,
                                             bool is_console,
                                             string_list const& hookdlls,
                                             const sub_process::env* penv) {
  sub_process* subprocess = new sub_process(is_console, raw_ptr);
  if (!subprocess->start(this, cmd_line, hookdlls, penv)) {
    delete subprocess;
    return 0;
  }
  on_new_subprocess(subprocess);
  return subprocess;
}

struct drive_info {
  string name;
  string path;
  int64_t total_space;
  int64_t available_space;
  int64_t write_speed;
  int64_t read_speed;
};

extern size_t get_drive_infos(vector<drive_info>& drives, bool test_speed = true);

wstring to_utf16(string const& str);
string to_utf8(wstring const& wstr);

class dynamic_library {
public:
  dynamic_library(const char* name);
  virtual ~dynamic_library();

  void* resolve_symbol(const char* symbol);

private:
  void* handle;
};

#if _WIN32
void wait_for_debugger();
/// <summary>
/// Returns is console
/// </summary>
/// <returns></returns>
bool check_attach_console();
string wc_to_utf8(const wchar_t* wstr);
void get_last_err(uint32_t& code, string& message);
namespace win {
class scoped_handle {
public:
  scoped_handle(void* in_handle) : m_handle(in_handle) {}
  ~scoped_handle() { close(); }
  void close();

private:
  void* m_handle;
};
} // namespace win
#endif
} // namespace iris
