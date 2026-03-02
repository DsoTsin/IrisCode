#pragma once

#include "ycore_config.h"
#include "compiler_support.h"
#include <stdint.h>

namespace iris {
class YCORE_API AssetCatalog {
public:
  AssetCatalog();
  ~AssetCatalog();

  bool load(const char* filename);
  void save(const char* filename);
};
} // namespace iris