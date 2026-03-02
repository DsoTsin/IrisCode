#pragma once
#include "stl.hpp"

namespace iris {
struct gpu_info {
  string name;
  string driver_version;
  uint64_t vram_size;
  uint64_t available_vram;
  uint32_t utilization;
};
struct machine_info {
  string computer_name;
  string computer_domain;
  string ip;
  uint64_t total_memory;
  uint64_t available_memory;
  uint64_t num_physical_cores;
  uint64_t num_logical_threads;
  uint64_t ddr_frequency;
  bool supports_avx512;
  int busy_percentage;
  vector<gpu_info> gpus;

  string to_json() const;
};
extern void get_machine_info(machine_info& info);
} // namespace iris