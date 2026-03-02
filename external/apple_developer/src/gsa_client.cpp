#include "gsa_client.h"
#include "anisette.h"
#include "authutils.h"
#include "exception.h"

namespace iris {
using namespace utility;           // Common utilities like string conversions
using namespace web;               // Common features like URIs.
using namespace web::http;         // Common HTTP functionality
using namespace web::http::client; // HTTP client features
using namespace concurrency::streams; // Asynchronous streams

struct GsaHttpConfig : public http_client_config {
  GsaHttpConfig() { set_validate_certificates(false); }
  ~GsaHttpConfig() {}
};

GsaClient::GsaClient()
    : http_client(U("https://gsa.apple.com"), GsaHttpConfig{}) {}
GsaClient::~GsaClient() {}

pplx::task<plist_t>
GsaClient::send_auth(const std::map<std::string, plist_t> &params) {
  auto header = plist_new_dict();
  plist_dict_set_item(header, "Version", plist_new_string("1.0.1"));

  auto requestDictionary = plist_new_dict();
  for (auto &parameter : params) {
    plist_dict_set_item(requestDictionary, parameter.first.c_str(),
                        parameter.second);
  }

  std::map<std::string, plist_t> parameters = {{"Header", header},
                                               {"Request", requestDictionary}};

  auto plist = plist_new_dict();
  for (auto &parameter : parameters) {
    plist_dict_set_item(plist, parameter.first.c_str(), parameter.second);
  }

  char *plistXML = nullptr;
  uint32_t length = 0;
  plist_to_xml(plist, &plistXML, &length);

  std::map<utility::string_t, utility::string_t> headers = {
      {L"Content-Type", L"text/x-xml-plist"},
      {L"X-Mme-Client-Info", to_utf16(Anisette::get().deviceDescription())},
      {L"Accept", L"*/*"},
      {L"User-Agent", L"akd/1.0 CFNetwork/978.0.7 Darwin/18.7.0"}};

  uri_builder builder(U("/grandslam/GsService2"));

  http_request request(methods::POST);
  request.set_request_uri(builder.to_string());
  request.set_body(plistXML);

  for (auto &pair : headers) {
    if (request.headers().has(pair.first)) {
      request.headers().remove(pair.first);
    }

    request.headers().add(pair.first, pair.second);
  }
  auto task =
      this->request(request)
          .then([](http_response response) { return response.content_ready(); })
          .then([](http_response response) {
#ifdef _DEBUG
            odslog("Received auth response status code: "
                   << response.status_code());
#endif
            return response.extract_vector();
          })
          .then([](std::vector<unsigned char> compressedData) {
#ifdef _DEBUG
            std::string resp =
                std::string(compressedData.begin(), compressedData.end());
#endif
            plist_t plist = nullptr;
            plist_from_xml((const char *)compressedData.data(),
                           (int)compressedData.size(), &plist);
            if (plist == nullptr) {
              throw Exception(ADSError::InvalidAuthenticateResponse,
                              "unable to parse plist response!");
            }

            return plist;
          })
          .then([](plist_t plist) {
            auto dictionary = plist_dict_get_item(plist, "Response");
            if (dictionary == NULL) {
              plist_free(plist);
              throw Exception(ADSError::InvalidAuthenticateResponse,
                              "Response not exists in plist!");
            }

            auto statusNode = plist_dict_get_item(dictionary, "Status");
            if (statusNode == NULL) {
              plist_free(plist);
              throw Exception(ADSError::InvalidAuthenticateResponse,
                              "Status not exists in plist!");
            }

            auto node = plist_dict_get_item(statusNode, "ec");
            int64_t resultCode = 0;

            auto type = plist_get_node_type(node);
            switch (type) {
            case PLIST_STRING: {
              char *value = nullptr;
              plist_get_string_val(node, &value);

              resultCode = atoi(value);
              break;
            }

            case PLIST_UINT: {
              uint64_t value = 0;
              plist_get_uint_val(node, &value);

              resultCode = (int64_t)value;
              break;
            }

            case PLIST_REAL: {
              double value = 0;
              plist_get_real_val(node, &value);

              resultCode = (int64_t)value;
              break;
            }

            default:
              break;
            }

            switch (resultCode) {
            case 0: {
              auto result = plist_copy(dictionary);
              plist_free(plist);
              return result;
            }
            case -29004:
              plist_free(plist);
              throw Exception(ADSError::InvalidAnisetteData,
                              "anisette data expired or invalid");
            default: {
              auto descriptionNode = plist_dict_get_item(statusNode, "em");
              if (descriptionNode == nullptr) {
                plist_free(plist);
                throw Exception(ADSError::InvalidAuthenticateResponse,
                                "em not exists in plist!");
              }

              char *errorDescription = nullptr;
              plist_get_string_val(descriptionNode, &errorDescription);

              if (errorDescription == nullptr) {
                plist_free(plist);
                throw Exception(ADSError::InvalidAuthenticateResponse,
                                "errorDescription not exists in plist!");
              }

              std::stringstream ss;
              ss << errorDescription << " (" << resultCode << ")";

              plist_free(plist);
              throw Exception(ADSError::InvalidAuthenticateResponse, ss.str());
            }
            }
          });

  free(plistXML);
  plist_free(plist);

  return task;
}

extern std::string kAppName;

pplx::task<std::vector<uint8_t>>
GsaClient::request_two_factor_auth(const std::string &adsid,
                                   const std::string &idms_token) {
  uri_builder builder(web::uri::encode_uri(L"/auth/verify/trusteddevice"));

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

  return this->request(request)
      .then([=](http_response response) { return response.content_ready(); })
      .then([=](http_response response) {
        odslog("Received 2FA response status code: " << response.status_code());
        return response.extract_vector();
      });
}
} // namespace iris