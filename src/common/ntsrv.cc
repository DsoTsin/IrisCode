#include "ntsrv.hpp"
#include "os.hpp"

#include <Windows.h>
#include <shlobj_core.h>

#define SERVICE_DEPENDENCIES ""
#define SERVICE_ACCOUNT "NT AUTHORITY\\LocalService" // TODO use login user, should use null
#define SERVICE_PASSWORD NULL

namespace iris {
namespace nt {
shared_ptr<service_controller_manager> g_scm;
service_controller_manager& service_controller_manager::get() {
  if (!g_scm) {
    g_scm.reset(new service_controller_manager);
  }
  return *g_scm;
}

shared_ptr<service_controller> service_controller_manager::create(const char* name,
                                                                  const char* disp_name,
                                                                  const char* binary_path) {
  if (err_code_)
    return nullptr;
  auto handle = CreateServiceA(
      (SC_HANDLE)sc_handle_,                                         // SCManager database
      name,                                                          // Name of service
      disp_name,                                                     // Name to display
      SERVICE_QUERY_STATUS,                                          // Desired access
      SERVICE_WIN32_SHARE_PROCESS /*| SERVICE_INTERACTIVE_PROCESS*/, // Service type
      SERVICE_AUTO_START,                                            // Service start type
      SERVICE_ERROR_NORMAL,                                          // Error control type
      binary_path,                                                   // Service's binary
      NULL,                                                          // No load ordering group
      NULL,                                                          // No tag identifier
      NULL,                                                          // Dependencies
      NULL,            // Service running account, default to localSystem
      SERVICE_PASSWORD // Password of the account
  );

  if (!handle) {
    err_code_ = GetLastError();
  }

  if (handle) {
    return make_shared<service_controller>(handle);
  }
  return nullptr;
}
shared_ptr<service_controller> service_controller_manager::open(const char* name) {
  if (err_code_)
    return nullptr;
  auto handle
      = OpenServiceA((SC_HANDLE)sc_handle_, name, SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);
  if (handle) {
    return make_shared<service_controller>(handle);
  }
  return nullptr;
}
service_controller_manager::~service_controller_manager() {
  CloseServiceHandle((SC_HANDLE)sc_handle_);
}
service_controller_manager::service_controller_manager() : err_code_(0) {
  if (is_an_admin()) {
    sc_handle_ = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  } else {
    sc_handle_ = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
  }

  if (!sc_handle_) {
    err_code_ = GetLastError();
  }
}
service_controller::service_controller(void* handle) : handle_(handle) {}
service_controller::~service_controller() {
  CloseServiceHandle((SC_HANDLE)handle_);
}
void service_controller::stop_service() {
  SERVICE_STATUS ssSvcStatus = {};
  ControlService((SC_HANDLE)handle_, SERVICE_CONTROL_STOP, &ssSvcStatus);
}
void service_controller::delete_service() {
  DeleteService((SC_HANDLE)handle_);
}
} // namespace nt
} // namespace iris
