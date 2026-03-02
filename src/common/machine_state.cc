#include "machine_state.hpp"
#include "os.hpp"
#include <json/writer.h>
#include <sstream>
#if _WIN32
#include "Windows.h"
#include <intrin.h>

typedef int* (*NvAPI_QueryInterface_t)(unsigned int offset);
typedef int (*NvAPI_Initialize_t)();
typedef int (*NvAPI_EnumPhysicalGPUs_t)(int** handles, int* count);
typedef int (*NvAPI_GPU_GetFullName_t)(int* handle, char* sysname);
typedef int (*NvAPI_GPU_GetPhysicalFrameBufferSize_t)(int* handle, int* memsize);
typedef int (*NvAPI_GPU_GetUsages_t)(int* Handle, unsigned int* Usages);
typedef struct {
  uint32_t version;                       //!< Structure version
  uint64_t dedicatedVideoMemory;          //!< Size(in bytes) of the physical framebuffer.
  uint64_t availableDedicatedVideoMemory; //!< Size(in bytes) of the available physical framebuffer
                                          //!< for
                                          //!< allocating video memory surfaces.
  uint64_t
      systemVideoMemory; //!< Size(in bytes) of system memory the driver allocates at load time.
  uint64_t sharedSystemMemory; //!< Size(in bytes) of shared system memory that driver is allowed to
                               //!< commit for surfaces across all allocations.
  uint64_t curAvailableDedicatedVideoMemory;  //!< Size(in bytes) of the current available physical
                                              //!< framebuffer for allocating video memory surfaces.
  uint64_t dedicatedVideoMemoryEvictionsSize; //!< Size(in bytes) of the total size of memory
                                              //!< released
                                              //!< as a result of the evictions.
  uint64_t dedicatedVideoMemoryEvictionCount; //!< Indicates the number of eviction events that
                                              //!< caused
                                              //!< an allocation to be removed from dedicated video
                                              //!< memory to free GPU video memory to make room for
                                              //!< other allocations.
  uint64_t dedicatedVideoMemoryPromotionsSize; //!< Size(in bytes) of the total size of memory
                                               //!< allocated as a result of the promotions.
  uint64_t dedicatedVideoMemoryPromotionCount; //!< Indicates the number of promotion events that
                                               //!< caused an allocation to be promoted to dedicated
                                               //!< video memory
} NV_GPU_MEMORY_INFO_EX_V1;
typedef NV_GPU_MEMORY_INFO_EX_V1 NV_GPU_MEMORY_INFO_EX;
//! \ingroup driverapi
#define MAKE_NVAPI_VERSION(typeName, ver) (uint32_t)(sizeof(typeName) | ((ver) << 16))
//! Macro for constructing the version field of NV_GPU_MEMORY_INFO_EX_V1
#define NV_GPU_MEMORY_INFO_EX_VER_1 MAKE_NVAPI_VERSION(NV_GPU_MEMORY_INFO_EX_V1, 1)

//! \ingroup driverapi
#define NV_GPU_MEMORY_INFO_EX_VER NV_GPU_MEMORY_INFO_EX_VER_1
typedef int (*NvAPI_GPU_GetMemoryInfoEx_t)(int* handle, NV_GPU_MEMORY_INFO_EX* memory_info);

struct nvapi {
  nvapi() {
    nvlib = ::LoadLibraryA("nvapi64.dll");
    if (nvlib) {
      auto NvAPI_QueryInterface
          = (NvAPI_QueryInterface_t)::GetProcAddress(nvlib, "nvapi_QueryInterface");
      NvAPI_Init = (NvAPI_Initialize_t)(*NvAPI_QueryInterface)(0x0150E828);
      NvAPI_EnumPhysicalGPUs = (NvAPI_EnumPhysicalGPUs_t)(*NvAPI_QueryInterface)(0xE5AC921F);
      NvAPI_GPU_GetUsages = (NvAPI_GPU_GetUsages_t)(*NvAPI_QueryInterface)(0x189A1FDF);
      NvAPI_GPU_GetFullName = (NvAPI_GPU_GetFullName_t)(NvAPI_QueryInterface)(0xCEEE8E9F);
      NvAPI_GPU_GetMemSize
          = (NvAPI_GPU_GetPhysicalFrameBufferSize_t)(NvAPI_QueryInterface)(0x46FBEB03);
      NvAPI_GPU_GetMemoryInfoEx = (NvAPI_GPU_GetMemoryInfoEx_t)(*NvAPI_QueryInterface)(0xc0599498);

      NvAPI_Init();
    }
  }

  ~nvapi() {
    if (nvlib) {
      FreeLibrary(nvlib);
      nvlib = nullptr;
    }
  }

  operator bool() const {
    return nvlib && NvAPI_GPU_GetFullName && NvAPI_GPU_GetMemSize && NvAPI_GPU_GetUsages;
  }

  bool enumerate(int& num, int* handles[64]) {
    if (NvAPI_EnumPhysicalGPUs && NvAPI_GPU_GetUsages) {
      (*NvAPI_EnumPhysicalGPUs)(handles, &num);
      return true;
    }
    return false;
  }

  bool get_name(int* handle, iris::string& name) {
    if (NvAPI_GPU_GetFullName) {
      char sysname[64] = {0};
      NvAPI_GPU_GetFullName(handle, sysname);
      name = sysname;
      return true;
    }
    return false;
  }

  bool get_mem_size(int* handle, uint64_t& avail_memsize, uint64_t& total_memsize) {
    if (NvAPI_GPU_GetMemSize) {
      NV_GPU_MEMORY_INFO_EX meminfo = {};
      meminfo.version = NV_GPU_MEMORY_INFO_EX_VER_1;
      NvAPI_GPU_GetMemoryInfoEx(handle, &meminfo);
      total_memsize = meminfo.dedicatedVideoMemory;
      avail_memsize = meminfo.curAvailableDedicatedVideoMemory;
      return true;
    }

    return false;
  }

  bool get_usage(int* handle, uint32_t& usage) {
    uint32_t usages[34] = {0};
    usages[0] = (136) | 0x10000;
    if (NvAPI_GPU_GetUsages) {
      NvAPI_GPU_GetUsages(handle, usages);
      usage = usages[3];
      return true;
    }
    return false;
  }

private:
  HMODULE nvlib;
  NvAPI_Initialize_t NvAPI_Init = nullptr;
  NvAPI_GPU_GetUsages_t NvAPI_GPU_GetUsages = nullptr;
  NvAPI_GPU_GetFullName_t NvAPI_GPU_GetFullName = nullptr;
  NvAPI_GPU_GetPhysicalFrameBufferSize_t NvAPI_GPU_GetMemSize = nullptr;
  NvAPI_GPU_GetMemoryInfoEx_t NvAPI_GPU_GetMemoryInfoEx = nullptr;
  NvAPI_EnumPhysicalGPUs_t NvAPI_EnumPhysicalGPUs = nullptr;
};

static nvapi _nvapi;
#endif

namespace iris {
void get_nv_gpu_info(vector<gpu_info>& gpu_infos) {
#if _WIN32
  int num_gpus = 0;
  int* gpu_handles[64];
  if (_nvapi) {
    _nvapi.enumerate(num_gpus, gpu_handles);
    for (int i = 0; i < num_gpus; i++) {
      gpu_info gpu;
      _nvapi.get_name(gpu_handles[i], gpu.name);
      _nvapi.get_usage(gpu_handles[i], gpu.utilization);
      _nvapi.get_mem_size(gpu_handles[i], gpu.available_vram, gpu.vram_size);
      gpu_infos.push_back(move(gpu));
    }
  }
#endif
}
void get_machine_info(machine_info& info) {
  info.computer_name = get_compute_name();
  // cpu
  info.num_logical_threads = get_num_cores();
  info.num_physical_cores = get_num_cores();
  info.busy_percentage = int(get_cpu_overall_usage());
  // memory
  info.total_memory = get_total_physical_memory();
  info.available_memory = get_available_physical_memory();
  info.ddr_frequency = 0;

#if defined(_M_X64)
  {
    int registers[4];
    __cpuid(registers, 0);
    uint32_t maximum_eax = registers[0];
    if (maximum_eax >= 7U) {
      __cpuidex(registers, 7, 0);
    }
    info.supports_avx512 = (registers[1] & (1 << 16)) != 0;
  }
#else
  info.supports_avx512 = false;
#endif
  // network
#if _WIN32
  {
    DWORD length = 0;
    ::GetComputerNameExA(ComputerNamePhysicalDnsFullyQualified, nullptr, &length);
    char* buffer = (char*)alloca(length + 1);
    ::GetComputerNameExA(ComputerNamePhysicalDnsFullyQualified, buffer, &length);
    info.computer_domain = buffer;
  }

  { info.ip; }
#endif

  get_nv_gpu_info(info.gpus);
}

string machine_info::to_json() const {
  Json::StreamWriterBuilder builder;
  const std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
  Json::Value master;
  master["name"] = computer_name;
  master["domain"] = computer_domain;
  master["total_mem"] = total_memory;
  master["avail_mem"] = available_memory;
  master["num_physical_cores"] = num_physical_cores;
  master["num_logical_cores"] = num_logical_threads;
  master["support_avx512"] = supports_avx512;
  master["cpu_overall_utilization"] = busy_percentage;
  Json::Value gpujs(Json::arrayValue);
  for (auto& gpu_info : gpus) {
    Json::Value item;
    item["name"] = gpu_info.name;
    item["vram_total"] = gpu_info.vram_size;
    item["vram_avail"] = gpu_info.available_vram;
    item["utilization"] = gpu_info.utilization;
    gpujs.append(item);
  }
  master["gpus"] = gpujs;
  std::ostringstream oss;
  writer->write(master, &oss);
  return oss.str();
}
} // namespace iris