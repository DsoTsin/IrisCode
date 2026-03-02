#pragma once

#include "authutils.h"

namespace iris {
class CodesignRequest {
public:
  CodesignRequest();
  ~CodesignRequest();

  const std::string &data() const { return data_; }
  const std::string &privateKey() const { return pkey_; }

private:
  std::string data_;
  std::string pkey_;
};
} // namespace iris