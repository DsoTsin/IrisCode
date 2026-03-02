#pragma once

#include "stl.hpp"

namespace iris {
namespace nt {
class service_controller;
class service_controller_manager {
private:
  void* sc_handle_;
  int err_code_;
  service_controller_manager();

public:
  static service_controller_manager& get();

  shared_ptr<service_controller> create(const char* name, const char* disp_name, const char* binary_path);
  shared_ptr<service_controller> open(const char* name);

  ~service_controller_manager();
};
class service_controller {
private:
  void* handle_;

public:
  service_controller(void* handle);
  ~service_controller();
  void stop_service();
  void delete_service();
};
} // namespace nt
} // namespace iris