#pragma once

#include "authutils.h"
#include "exception.h"
#include "developer.h"

namespace iris {
class GsaClient : public web::http::client::http_client {
public:
  GsaClient();
  ~GsaClient();

  pplx::task<plist_t> authenticate(const std::string &account,
                                   const std::string &password);

  pplx::task<std::vector<uint8_t>> verify(const std::string &code,
                                          const std::string &adsid,
                                          const std::string &idms_token);

  pplx::task<std::string> fetch_auth_token(const bytes_view &sk,
                                           const bytes_view &c,
                                           const std::string &adsid,
                                           const std::string &idms_token);

private:
  pplx::task<plist_t> send_auth(const std::map<std::string, plist_t> &params);

  pplx::task<bytes> request_two_factor_auth(const std::string &adsid,
                                            const std::string &idms_token);
};

} // namespace iris