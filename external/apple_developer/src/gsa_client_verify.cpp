#include "anisette.h"
#include "authutils.h"
#include "gsa_client.h"

namespace iris {
using namespace utility;           // Common utilities like string conversions
using namespace web;               // Common features like URIs.
using namespace web::http;         // Common HTTP functionality
using namespace web::http::client; // HTTP client features
using namespace concurrency::streams; // Asynchronous streams

extern std::string kAppName;

pplx::task<std::vector<uint8_t>>
GsaClient::verify(const std::string &code, const std::string &adsid,
                  const std::string &idms_token) {
  uri_builder builder(web::uri::encode_uri(L"/grandslam/GsService2/validate"));
  http_request request(methods::GET);
  request.set_request_uri(builder.to_string());
  std::string identityToken = adsid + ":" + idms_token;
  auto header_dict = Anisette::get().make_string_dict();
  header_dict.insert({"Content-Type", "text/x-xml-plist"});
  header_dict.insert({"User-Agent", "Xcode"});
  header_dict.insert({"Accept", "text/x-xml-plist"});
  header_dict.insert({"Accept-Language", "en-us"});
  header_dict.insert({"X-Apple-App-Info", kAppName});
  header_dict.insert({"X-Xcode-Version", "11.2 (11B41)"});
  header_dict.insert(
      {"X-Apple-Identity-Token", // base64 encode
       base64((const uint8_t *)identityToken.c_str(), identityToken.length())});
  for (auto &pair : header_dict) {
    auto key = to_utf16(pair.first);
    auto value = to_utf16(pair.second);
    if (request.headers().has(key)) {
      request.headers().remove(key);
    }
    request.headers().add(key, value);
  }
  request.headers().add(L"security-code", to_utf16(code));
  return this->request(request)
      .then([](http_response response) { return response.content_ready(); })
      .then([](http_response response) {
        odslog(
            "Received auth security-code response status code: " << response.status_code());
        return response.extract_vector();
      }); // TODO: error verify code
}
} // namespace iris