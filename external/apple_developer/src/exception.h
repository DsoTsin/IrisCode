#pragma once

#include "developer.h"

namespace iris {
enum class ADSError : int {
  InternalError =
      ::apache::thrift::TApplicationException::UNSUPPORTED_CLIENT_TYPE + 1,
  LibraryLoadError,
  InvalidAnisetteData,
  InvalidAuthenticateResponse,
  ServiceError,
};

class Exception : public error {
public:
  Exception(ADSError err, std::string const &detail = "") {
    code = (int16_t)err;
    description = detail;
  }

};
} // namespace iris