#include "download.hpp"
#include "base64.hpp"
#include "log.hpp"
#include "md5.hpp"
#include "os.hpp"
#include <ext/socket.hpp>
#include <fstream>
#include <sstream>
#include <ctime>
#include <locale>
#include <iomanip>

#if _WIN32
#include <Windows.h>
#include <VersionHelpers.h>
#include <winhttp.h>
#define USE_WIN_HTTP 1
#define USE_CURL 0
#else
#include <curl/curl.h>
#define USE_CURL 1
#endif

namespace iris {
string get_host_name(string const& url, bool& use_https, string& component) {
  string host;
  string::size_type domain_end = string::npos;
  if (url.find("https://") == 0) {
    host = url.substr(8, url.length() - 8);
    domain_end = url.find_first_of('/', 8);
    if (domain_end != string::npos) {
      host = url.substr(8, domain_end - 8);
    }
    use_https = true;
  } else if (url.find("http://") == 0) {
    host = url.substr(7, url.length() - 7);
    domain_end = url.find_first_of('/', 7);
    if (domain_end != string::npos) {
      host = url.substr(7, domain_end - 7);
    }
    use_https = false;
  }
  component = url.substr(domain_end, url.length() - domain_end);
  return host;
}

#if _WIN32
class WinHttpHandle {
public:
  WinHttpHandle(HINTERNET handle) : m_handle(handle) {}
  ~WinHttpHandle() {
    if (m_handle) {
      WinHttpCloseHandle(m_handle);
      m_handle = NULL;
    }
  }
  operator HINTERNET() const { return m_handle; }

private:
  HINTERNET m_handle;
};

bool query_header_value(HINTERNET hRequest, DWORD name, string& value) {
  DWORD dwSize = 0;
  vector<WCHAR> str_buffer;
  auto bResults = WinHttpQueryHeaders(hRequest, name, WINHTTP_HEADER_NAME_BY_INDEX,
                                 NULL, &dwSize, WINHTTP_NO_HEADER_INDEX);
  if (dwSize > 0) {
    str_buffer.resize(dwSize / sizeof(WCHAR));
    bResults = WinHttpQueryHeaders(hRequest, name, WINHTTP_HEADER_NAME_BY_INDEX, str_buffer.data(),
                                   &dwSize, WINHTTP_NO_HEADER_INDEX);
  }
  if (bResults == FALSE) {
    return false;
  }
  value = to_utf8(str_buffer.data());
  return true;
}

bool parse_datetime_to_filetime(const char* str, FILETIME& ft) {
  int day, year, hour, min, sec;
  char monthStr[4] = {0}; // Holds 3-character month + null terminator
  // Parse the input string using sscanf_s
  int parsed = sscanf_s(str, "%*3s, %d %3s %d %d:%d:%d %*3s", &day, monthStr,
                        static_cast<unsigned>(_countof(monthStr)), &year, &hour, &min, &sec);
  if (parsed != 6) { // Ensure all 6 components were parsed
    return false;
  }
  // Map month abbreviation to numeric value (1-12)
  const char* months[]
      = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  int month = 0;
  for (int i = 0; i < 12; ++i) {
    if (strcmp(monthStr, months[i]) == 0) {
      month = i + 1; // Months are 1-based
      break;
    }
  }
  if (month == 0) { // Invalid month abbreviation
    return false;
  }
  // Populate SYSTEMTIME structure
  SYSTEMTIME st = {0};
  st.wYear = static_cast<WORD>(year);
  st.wMonth = static_cast<WORD>(month);
  st.wDay = static_cast<WORD>(day);
  st.wHour = static_cast<WORD>(hour);
  st.wMinute = static_cast<WORD>(min);
  st.wSecond = static_cast<WORD>(sec);
  st.wMilliseconds = 0; // Not provided in input
  // Convert SYSTEMTIME to FILETIME (UTC)
  return SystemTimeToFileTime(&st, &ft);
}

download_status winhttp_download(string const& url,
                      string const& file_path,
                      fn_download_progress prog,
                      void* data) {
  string file_md5 = path::file_content_md5(file_path);
  DWORD access = IsWindows8OrGreater() ? WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY
                                       : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
  auto hSession = WinHttpHandle(
      WinHttpOpen(L"XBuild/1.0", access, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
  DWORD secure_protocols(WINHTTP_FLAG_SECURE_PROTOCOL_SSL3 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1
                         | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1
                         | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2);
  WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, &secure_protocols,
                   sizeof(secure_protocols));
  bool use_https = false;
  string component;
  string host = get_host_name(url, use_https, component);
  auto hConnect = WinHttpHandle(
      WinHttpConnect(hSession, to_utf16(host).c_str(),
                     use_https ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0));
  auto hRequest = WinHttpHandle(
      WinHttpOpenRequest(hConnect, L"GET", to_utf16(component).c_str(), nullptr, WINHTTP_NO_REFERER,
                         WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE));
  auto bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                     WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
  if (bResults == FALSE) {
    XB_LOGE("error: download failed : request error, %s ", url.c_str());
    return download_status::failed;
  }
  bResults = WinHttpReceiveResponse(hRequest, NULL);
  if (bResults == FALSE) {
    XB_LOGE("error: download failed : receive response error, %s ", url.c_str());
    return download_status::failed;
  }
  std::vector<char> buf;
  size_t total_downloaded_size = 0;
  string length_str;
  if (!query_header_value(hRequest, WINHTTP_QUERY_CONTENT_LENGTH, length_str)) {
    XB_LOGE("error: download failed : failed to get content-length: %s ", url.c_str());
    return download_status::failed;
  }
  auto content_length = stoll(length_str);
  XB_LOGI("Length = %d, %s", content_length, url.c_str());
  string md5_base64;
  if (!query_header_value(hRequest, WINHTTP_QUERY_CONTENT_MD5, md5_base64)) {
    XB_LOGE("error: download failed : failed to get content-md5: %s ", url.c_str());
  } else {
    XB_LOGI("MD5 = %s, %s", md5_base64.c_str(), url.c_str());
  }
  string content_disposition;
  string filename;
  if (!query_header_value(hRequest, WINHTTP_QUERY_CONTENT_DISPOSITION, content_disposition)) {
    XB_LOGE("error: download failed : failed to get content-disposition: %s ", url.c_str());
  } else {
    auto filename_pos = content_disposition.find("filename=");
    if (filename_pos != std::string::npos) {
      filename = content_disposition.substr(filename_pos + 9);
      if (filename.length() > 2 && filename[0] == '"' && filename[filename.length() - 1] == '"') {
        filename = filename.substr(1, filename.length() - 2);
      }
      XB_LOGI("FileName = %s, %s", filename.c_str(), url.c_str());
    }
  }
  string eTag;
  if (!query_header_value(hRequest, WINHTTP_QUERY_ETAG, eTag)) {
    XB_LOGE("error: download failed : failed to get eTag: %s ", url.c_str());
  } else {
    XB_LOGI("eTag: %s, url = %s", eTag.c_str(), url.c_str());
  }
  string lastModified;
  FILETIME ft = {};
  if (!query_header_value(hRequest, WINHTTP_QUERY_LAST_MODIFIED, lastModified)) {
    XB_LOGE("error: download failed : failed to get Last Modified: %s ", url.c_str());
  } else {
    if (!parse_datetime_to_filetime(lastModified.c_str(), ft)) {
      XB_LOGW("Failed to convert '%s' to FILETIME", lastModified.c_str());
    }
  }

  auto fdir = path::file_dir(file_path);
  if (!path::exists(fdir)) {
    path::make(fdir);
  }
  string download_path = file_path;
  if ((GetFileAttributesA(file_path.c_str()) & FILE_ATTRIBUTE_DIRECTORY) != 0 && !filename.empty()) {
    if (download_path.endswith("/") || download_path.endswith("\\")) {
      download_path += filename;
    } else {
      download_path = download_path + "/" + filename;
    }
    file_md5 = path::file_content_md5(download_path);
  }

  {
    string range_type;
    if (!query_header_value(hRequest, WINHTTP_QUERY_ACCEPT_RANGES, eTag)) {
      XB_LOGW("warning: range request not supported, %s ", url.c_str());
    } else {
      if (range_type == "bytes") {
        XB_LOGI("range request supported, %s", url.c_str());
      } else if (range_type == "none") {
        XB_LOGI("warning: range request not supported, %s", url.c_str());
      }
    }
  }
  bool need_redownload = true;
  if (!md5_base64.empty()) {
    string md5_raw = decode_base64(md5_base64);
    string real_md5 = MD5::Str(md5_raw);
    if (!file_md5.empty() && real_md5 == file_md5) {
      need_redownload = false;
    }
  }
  if (need_redownload) {
    HANDLE filehandle = CreateFileA(download_path.c_str(), GENERIC_WRITE | GENERIC_READ,
                          FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL, NULL);
    if (filehandle == INVALID_HANDLE_VALUE) {
      XB_LOGE("error: create file at '%s' failed, url: %s", download_path.c_str(), url.c_str());
      return download_status::failed;
    }
    DWORD dwSize = 0;
    std::vector<uint8_t> fix_buffer;
    fix_buffer.resize(256 * 1024); // Default buffer 256K
    do {
      DWORD downloaded_size = 0;
      bResults = WinHttpQueryDataAvailable(hRequest, &dwSize);
      if (bResults != TRUE) {
        XB_LOGE("error: WinHttpQueryDataAvailable() failed: %d, url: %s", GetLastError(),
                url.c_str());
        return download_status::failed;
      }
#if 0
      if (buf.size() < dwSize)
        buf.resize(dwSize * 2);
      bResults = WinHttpReadData(hRequest, (LPVOID)buf.data(), dwSize, &downloaded_size);
#else
      bResults = WinHttpReadData(hRequest, (LPVOID)fix_buffer.data(), fix_buffer.size(),
                                 &downloaded_size);
#endif
      if (bResults != TRUE) {
        XB_LOGE("error: WinHttpReadData() failed: %d, url: %s", GetLastError(), url.c_str());
        return download_status::failed;
      }
      if (prog) {
        prog(download_path.c_str(), downloaded_size, total_downloaded_size, content_length, data);
      }
#if 0
      of.write(buf.data(), downloaded_size);
#else
      DWORD written = 0;
      if (FALSE == ::WriteFile(filehandle, fix_buffer.data(), downloaded_size, &written, NULL)) {
        XB_LOGE("error: WriteFile() failed: %d, url: %s", GetLastError(), url.c_str());
        break;
      }
      FlushFileBuffers(filehandle);
#endif
      total_downloaded_size += downloaded_size;
    } while (dwSize > 0);
    SetFileTime(filehandle, &ft, &ft, &ft);
    CloseHandle(filehandle);
    return download_status::succeed;
  } else {
    return download_status::md5_unchanged;
  }
}
#endif

struct http_header {
  string get(string const& key) const {
    auto iter = m_header_data.find(key);
    if (iter == m_header_data.end())
      return "";
    return iter->second;
  }
  http_header& add(string const& key, string const& val) {
    m_header_data[key] = val;
    return *this;
  }

private:
  unordered_map<string, string> m_header_data;
};

struct http_response {
  int code;
  http_header header;
  string content_md5_str;
  string temp_content;
};

download_status download_url(string const& url,
                  string const& file_path,
                  fn_download_progress prog,
                  void* data) {
#if USE_WIN_HTTP
  return winhttp_download(url, file_path, prog, data);
#else
  downloader d;
  d.download(url, file_path);
#endif
}

struct _downloader_impl {
  _downloader_impl() : sock(nullptr) {
#if HAS_OPENSSL
    m_ssl_factory = make_shared<ext::ssl_socket_factory>();
#endif
  }

  ~_downloader_impl() {
    if (sock) {
      delete sock;
      sock = nullptr;
    }
  }

  void get(string const& url, string const& file_path, string const& cur_md5, http_response& resp) {
    bool use_https = false;
    string component;
    string host = get_host_name(url, use_https, component);
    if (use_https) {
#if HAS_OPENSSL
      sock = m_ssl_factory->connect_to_host(host, 443);
#endif
    } else {
      sock = ext::socket::connect_to_host(host, 80);
    }
    // GET request
    get_impl(component, host, file_path, cur_md5, resp);
  }

  void get_impl(string const& req,
                string const& host,
                string const& file_path,
                string const& cur_md5,
                http_response& resp) {
    ostringstream httpdata;
    httpdata << "GET " << req << " HTTP/1.1\r\n";
    httpdata << "Host: " << host << "\r\n";
    httpdata << "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:57.0) Gecko/20100101 "
                "Firefox/57.0\r\n";
    httpdata << "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
    httpdata << "Accept-Language: en-US,en;q=0.5\r\n";
    httpdata << "Accept-Encoding: gzip, deflate, br\r\n";
    httpdata << "Connection: keep-alive\r\n";
    httpdata << "Upgrade-Insecure-Requests: 1\r\n\r\n";
    int sent = sock->send(httpdata.str().data(), (int)httpdata.str().length());
    parse_resp_header(resp);
    switch (resp.code) {
    case 200: {
      receive_content(file_path, cur_md5, resp);
      break;
    }
    case 302: {
      string reurl = resp.header.get("Location");
      redirect(reurl, file_path, cur_md5, resp);
    }
    default:
      break;
    }
  }

  void parse_resp_header(http_response& resp) {
    char buffer[4096] = {0};
    int recved = sock->recv(buffer, 4096);
    if (recved <= 4096 && recved > 0) {
      string temp_resp = buffer;
      auto header_end = temp_resp.find("\r\n\r\n");
      string header = temp_resp.substr(0, header_end);
      auto fle = header.find("\r\n");
      string first_line = header.substr(0, fle);
      header = header.substr(fle + 2, header.length() - fle - 2);
      parse_resp_code(first_line, resp.code);
      parse_resp_header(header, resp.header);
      if (recved > header_end + 4) {
        auto temp_length = recved - header_end - 4;
        resp.temp_content.resize(temp_length);
        memcpy(&resp.temp_content[0], buffer + header_end + 4, temp_length);
      }
    }
  }

  void parse_resp_code(string const& line, int& code) {
    auto fsp = line.find_first_of(' ');
    auto ssp = line.find_first_of(' ', fsp + 1);
    string code_str = line.substr(fsp, ssp - fsp);
    code = atoi(code_str.c_str());
  }

  void parse_resp_header(string const& header_data, http_header& header) {
    string::size_type he = 0;
    string::size_type hs = 0;
    do {
      string line;
      he = header_data.find("\r\n", he);
      if (he != string::npos) {
        line = header_data.substr(hs, he - hs);
        he += 2;
        hs = he;
      } else {
        line = header_data.substr(hs, header_data.length() - hs);
      }
      auto ke = line.find_first_of(':');
      if (line[ke + 1] == ' ') {
        header.add(line.substr(0, ke), line.substr(ke + 2, line.length() - ke - 2));
      } else {
        header.add(line.substr(0, ke), line.substr(ke + 1, line.length() - ke - 1));
      }
    } while (he != string::npos);
  }

  void receive_content(string const& file_path, string const& cur_md5, http_response& resp) {
    sock->set_blocking(true);
    string length = resp.header.get("Content-Length");
    string c_md5 = resp.header.get("Content-MD5");
    string md5_raw = decode_base64(c_md5);
    resp.content_md5_str = MD5::Str(md5_raw);
    if (cur_md5 == resp.content_md5_str) {
      XB_LOGI("file (%s) md5 unchanged, won't download.", file_path.c_str());
      return;
    }
    long long c_length = atoll(length.c_str());
    long long c_remain = c_length - resp.temp_content.size();
    ofstream save_file(file_path, ios::binary);
    save_file.write(resp.temp_content.data(), resp.temp_content.size());
    long long slice_sz = 512 * 1024;
    string buffer(512 * 1024, '\0');
    int recv = 0;
    while ((recv = sock->recv((char*)buffer.data(), (int)slice_sz)) > 0) {
      if (c_remain > 0) {
        c_remain -= recv;
      }
      save_file.write(buffer.data(), recv);
      // resp.temp_content.append(buffer.data(), recv);
      if (c_remain < slice_sz) {
        slice_sz = c_remain;
      }
      if (c_remain == 0) {
        break;
      }
    }
    // finished
    save_file.close();
  }

  void redirect(string const& url,
                string const& file_path,
                string const& cur_md5,
                http_response& resp) {
    string redirect_url = resp.header.get("Location");
    if (!redirect_url.empty()) {
      auto dimpl = make_shared<_downloader_impl>();
      http_response new_resp = {0, http_header(), "", ""};
      dimpl->get(redirect_url, file_path, cur_md5, new_resp);
    }
  }

#if HAS_OPENSSL
  shared_ptr<ext::ssl_socket_factory> m_ssl_factory;
#endif
  ext::socket* sock;
};

downloader::downloader(option opt) : m_impl(new _downloader_impl) {}
download_status downloader::download(string const& url, string const& target_file) {
  string cur_md5;
  if (path::exists(target_file)) {
    cur_md5 = path::file_content_md5(target_file);
  }
  http_response resp = {0, http_header(), "", ""};
  m_impl->get(url, target_file, cur_md5, resp);
  return download_status::succeed;
}
#if USE_CURL
namespace curl {
class session {
private:
  CURL* handle_;

public:
  session();
  ~session();
};

class download_manager {
public:
  static download_manager& g() {
    static download_manager dm;
    return dm;
  }

private:
  download_manager();
  ~download_manager();
};
} // namespace curl

namespace curl {
session::session() : handle_(NULL) {
  handle_ = ::curl_easy_init();
  // todo proxy, range
  curl_easy_setopt(handle_, CURLOPT_WRITEFUNCTION, nullptr /*write function*/);
  curl_easy_setopt(handle_, CURLOPT_WRITEDATA, nullptr /*objecct*/);
  curl_easy_setopt(handle_, CURLOPT_NOPROGRESS, 1L);
  curl_easy_setopt(handle_, CURLOPT_URL, "" /*url*/);
  curl_easy_setopt(handle_, CURLOPT_VERBOSE, 1L);
  curl_easy_perform(handle_);
}

session::~session() {
  if (handle_) {
    ::curl_easy_cleanup(handle_);
    handle_ = nullptr;
  }
}

download_manager::download_manager() {
  curl_global_init(CURL_GLOBAL_ALL);
  // todo print
  curl_version();
}
download_manager::~download_manager() {
  curl_global_cleanup();
}
} // namespace curl
#endif
} // namespace iris
