#include "service_client.h"
#include "anisette.h"
#include "authutils.h"
#include "csr.h"

namespace iris {
using namespace utility;           // Common utilities like string conversions
using namespace web;               // Common features like URIs.
using namespace web::http;         // Common HTTP functionality
using namespace web::http::client; // HTTP client features
using namespace concurrency::streams; // Asynchronous streams

extern std::string kAppName;

#define USE_DEVELOPER_SERVICE
#ifndef USE_DEVELOPER_SERVICE
#define SVR_ENDPOINT "https://developer.apple.com/services-account/v1/"
#else
#define SVR_ENDPOINT "https://developerservices2.apple.com/services/v1"
#endif

ServiceClient::ServiceClient() : http_client(U(SVR_ENDPOINT)) {}

ServiceClient::~ServiceClient() {}

pplx::task<json::value> ServiceClient::send_request(
    const std::string &uri, const std::string &method,
    const std::string &team_id, const std::string &dsid,
    const std::string &auth_token,
    const std::map<std::string, std::string> &parameters) {
  auto encodedParametersURI = web::uri::encode_uri(L"");
  uri_builder parametersBuilder(encodedParametersURI);
  parametersBuilder.append_query(L"teamId", to_utf16(team_id), true);

  for (auto &pair : parameters) {
    parametersBuilder.append_query(to_utf16(pair.first), to_utf16(pair.second),
                                   true);
  }

  auto query = parametersBuilder.query();

  auto json = web::json::value::object();
  json[L"urlEncodedQueryParams"] = web::json::value::string(query);

  utility::stringstream_t stream;
  json.serialize(stream);

  auto jsonString = to_utf8(stream.str()); // TODO to_utf8

  auto wideURI = to_utf16(uri);
  auto encodedURI = web::uri::encode_uri(wideURI);
  uri_builder builder(encodedURI);

  http_request request(methods::POST);
  request.set_request_uri(builder.to_string());
  request.set_body(jsonString);

  auto header = Anisette::get().make_string_dict(false);
  header.insert({
      {"Content-Type", "application/vnd.api+json"},
      {"User-Agent", "Xcode"},
      {"Accept", "application/vnd.api+json"},
      {"Accept-Language", "en-us"},
      {"X-Apple-App-Info", kAppName},
      {"X-Xcode-Version", "11.2 (11B41)"},
      {"X-HTTP-Method-Override", method},
      {"X-Apple-I-Identity-Id", dsid},
      {"X-Apple-GS-Token", auth_token},
  });

  for (auto &pair : header) {
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
        odslog("Received response status code: " << response.status_code());
        return response.extract_vector();
      })
      .then([=](std::vector<unsigned char> decompressedData) {
        std::string decompressedJSON =
            std::string(decompressedData.begin(), decompressedData.end());

        if (decompressedJSON.size() == 0) {
          return json::value::object();
        }

        utility::stringstream_t s;
        if (decompressedJSON.size() > 1 && decompressedJSON[0] == '{') {
          s << to_utf16(decompressedJSON);
        } else {
          gzip_decompress(bytes_view((const uint8_t *)decompressedData.data(),
                                     (size_t)decompressedData.size()),
                          decompressedJSON);
          s << to_utf16(decompressedJSON);
        }

        auto json = json::value::parse(s);
        int64_t resultCode = 0;
        if (json.has_field(L"resultCode")) {
          resultCode = json[L"resultCode"].as_integer();
        } else if (json.has_field(L"errorCode")) {
          resultCode = json[L"errorCode"].as_integer();
        } else {
          resultCode = -1;
        }

        std::string errorDescription;
        if (json.has_field(L"userString")) {
          errorDescription = to_utf8(json[L"userString"].as_string());
        } else if (json.has_field(L"resultString")) {
          errorDescription = to_utf8(json[L"resultString"].as_string());
        } else if (json.has_field(L"errorMessage")) {
          errorDescription = to_utf8(json[L"errorMessage"].as_string());
        } else if (json.has_field(L"errorId")) {
          errorDescription = to_utf8(json[L"errorId"].as_string());
        } else {
          errorDescription = "Unknown services response error.";
        }

        if (resultCode != 0 && resultCode != -1) {
          std::stringstream ss;
          ss << errorDescription << " (" << resultCode << ")";
          throw Exception(ADSError::ServiceError, ss.str());
        }
        return json;
      });
}

ServiceClient::ServiceClient(const web::uri &uri) : http_client(uri) {}

std::string kAuthenticationProtocolVersion = "A1234";
std::string kProtocolVersion = "QH65B2";
std::string kAppIDKey =
    "ba2ec180e6ca6e6c6a542255453b24d6e6e5b2be0cc48bc1b0d8ad64cfe0228f";
std::string kClientID = "XABBG36SBA";

ServiceClientQH65B2::ServiceClientQH65B2()
    : ServiceClient(U("https://developerservices2.apple.com/services/QH65B2")) {
}

ServiceClientQH65B2::~ServiceClientQH65B2() {}

pplx::task<std::optional<developer_account>>
ServiceClientQH65B2::fetch_account(const std::string &adsid,
                                   const std::string &auth_token) {
  return this->send_request("viewDeveloper.action", "", adsid, auth_token, {})
      .then([](plist_t plist) {
        auto result = plist_dict_get_item(plist, "resultCode");
        std::optional<developer_account> result_account = std::nullopt;
        auto developer = plist_dict_get_item(plist, "developer");
        if (developer) {
          developer_account account;
          auto developer_id = plist_dict_get_item(developer, "developerId");
          if (developer_id) {
            char *val = nullptr;
            plist_get_string_val(developer_id, &val);
            account.identifier = val;
            free(val);
          }
          // auto person_id = plist_dict_get_item(developer, "personId");
          auto first_name = plist_dict_get_item(developer, "firstName");
          if (first_name) {
            char *val = nullptr;
            plist_get_string_val(first_name, &val);
            account.first_name = val;
            free(val);
          }
          auto last_name = plist_dict_get_item(developer, "lastName");
          if (last_name) {
            char *val = nullptr;
            plist_get_string_val(last_name, &val);
            account.last_name = val;
            free(val);
          }
          // auto email = plist_dict_get_item(developer, "email");
          auto status = plist_dict_get_item(developer, "developerStatus");
          if (status) {
            char *val = nullptr;
            plist_get_string_val(status, &val);
            account.status = val;
            free(val);
          }
          result_account = std::make_optional(account);
        }
        plist_free(plist);
        return result_account;
      });
}

pplx::task<std::vector<developer_team>>
ServiceClientQH65B2::fetch_teams(const std::string &dsid,
                                 const std::string &auth_token) {
  return this->send_request("listTeams.action", "", dsid, auth_token, {})
      .then([](plist_t plist) {
        std::vector<developer_team> teams;
        auto nteams = plist_dict_get_item(plist, "teams");
        if (nteams) {
          int num_teams = plist_array_get_size(nteams);
          for (int i = 0; i < num_teams; i++) {
            auto nteam = plist_array_get_item(nteams, i);
            if (nteam) {
              developer_team team;
              auto nstatus = plist_dict_get_item(nteam, "status");
              auto nname = plist_dict_get_item(nteam, "name");
              GET_NODE_STR(team.name, nname)
              auto nteam_id = plist_dict_get_item(nteam, "teamId");
              GET_NODE_STR(team.id, nteam_id)
              auto ntype = plist_dict_get_item(nteam, "type");
              auto ncmem = plist_dict_get_item(nteam, "currentTeamMember");
              if (ncmem) {
                auto aroles = plist_dict_get_item(ncmem, "roles");
                if (aroles) {
                  int num_roles = plist_array_get_size(aroles);
                  for (int rid = 0; rid < num_roles; rid++) {
                    auto nrole = plist_array_get_item(aroles, rid);
                    char *role = nullptr;
                    plist_get_string_val(nrole, &role);

                    free(role);
                  }
                }
              } // ncmem
              teams.push_back(team);
            } // nteam
          }

        } // nteams
        plist_free(plist);
        return teams;
      });
}

pplx::task<std::vector<device>>
ServiceClientQH65B2::fetch_devices(const std::string &team_id,
                                   const std::string &dsid,
                                   const std::string &auth_token) {
  return this
      ->send_request("ios/listDevices.action", team_id, dsid, auth_token, {})
      .then([](plist_t plist) {
        std::vector<device> devices;
        auto ndevs = plist_dict_get_item(plist, "devices");
        if (ndevs) {
          int num_devs = plist_array_get_size(ndevs);
          for (int i = 0; i < num_devs; i++) {
            device dev;
            auto ndev = plist_array_get_item(ndevs, i);
            if (ndev) {
              auto nname = plist_dict_get_item(ndev, "name");
              GET_NODE_STR(dev.name, nname)
              auto nid = plist_dict_get_item(ndev, "deviceId");
              GET_NODE_STR(dev.id, nid)
              auto ndno = plist_dict_get_item(ndev, "deviceNumber");
              auto ndp = plist_dict_get_item(ndev, "devicePlatform");
              auto nmd = plist_dict_get_item(ndev, "model");

              devices.push_back(std::move(dev));
            } // end dev
          }
        }
        plist_free(plist);
        return devices;
      });
}

void ServiceClientQH65B2::register_device(const std::string &dev_name,
                                          const std::string &dev_identifier,
                                          const device_type::type &dev_type,
                                          const std::string &team_id,
                                          const std::string &dsid,
                                          const std::string &auth_token) {

  std::map<std::string, plist_t> parameters = {
      {"name", plist_new_string(dev_name.c_str())},
      {"deviceNumber", plist_new_string(dev_identifier.c_str())}};

  switch (dev_type) {
  case device_type::iphone:
  case device_type::ipad:
    parameters["DTDK_Platform"] = plist_new_string("ios");
    break;

  case device_type::tv:
    parameters["DTDK_Platform"] = plist_new_string("tvos");
    parameters["subPlatform"] = plist_new_string("tvOS");
    break;

  default:
    break;
  }

  this->send_request("ios/addDevice.action", team_id, dsid, auth_token,
                     parameters)
      .then([](plist_t plist) {
        std::vector<device> devices;
        plist_free(plist);
        return devices;
      });
}
pplx::task<plist_t> ServiceClientQH65B2::send_request(
    const std::string &uri, const std::string &team_id, const std::string &dsid,
    const std::string &auth_token,
    const std::map<std::string, plist_t> &additional_params) {
  std::string requestID(make_uuid());

  std::map<std::string, plist_t> parameters = {
      {"clientId", plist_new_string(kClientID.c_str())},
      {"protocolVersion", plist_new_string(kProtocolVersion.c_str())},
      {"requestId", plist_new_string(requestID.c_str())},
  };

  auto plist = plist_new_dict();
  for (auto &parameter : parameters) {
    plist_dict_set_item(plist, parameter.first.c_str(), parameter.second);
  }

  for (auto &parameter : additional_params) {
    plist_dict_set_item(plist, parameter.first.c_str(),
                        plist_copy(parameter.second));
  }

  if (!team_id.empty()) {
    plist_dict_set_item(plist, "teamId", plist_new_string(team_id.c_str()));
  }

  char *plistXML = nullptr;
  uint32_t length = 0;
  plist_to_xml(plist, &plistXML, &length);

  auto wideURI = to_utf16(uri);
  auto wideClientID = to_utf16(kClientID);

  auto encodedURI = web::uri::encode_uri(wideURI);
  uri_builder builder(encodedURI);

  http_request request(methods::POST);
  request.set_request_uri(builder.to_string());
  request.set_body(plistXML);

  auto header = Anisette::get().make_string_dict();
  header.insert({
      {"Content-Type", "text/x-xml-plist"},
      {"User-Agent", "Xcode"},
      {"Accept", "text/x-xml-plist"},
      {"Accept-Language", "en-us"},
      {"X-Apple-App-Info", kAppName},
      {"X-Xcode-Version", "11.2 (11B41)"},

      {"X-Apple-I-Identity-Id", dsid},
      {"X-Apple-GS-Token", auth_token},
  });

  for (auto &pair : header) {
    auto key = to_utf16(pair.first);
    auto value = to_utf16(pair.second);
    if (request.headers().has(key)) {
      request.headers().remove(key);
    }

    request.headers().add(key, value);
  }

  auto task =
      this->request(request)
          .then(
              [=](http_response response) { return response.content_ready(); })
          .then([=](http_response response) {
#ifdef _DEBUG
            odslog("Received response status code: " << response.status_code());
#endif
            return response.extract_vector();
          })
          .then([=](bytes compressedData) {
            std::string decompressedData;

            if (compressedData.size() > 2 && compressedData[0] == '<' &&
                compressedData[1] == '?') {
              decompressedData =
                  std::string(compressedData.begin(), compressedData.end());
            } else {
              gzip_decompress(bytes_view((const uint8_t *)compressedData.data(),
                                         (size_t)compressedData.size()),
                              decompressedData);
            }

            plist_t plist = nullptr;
            plist_from_xml((const char *)decompressedData.data(),
                           (int)decompressedData.size(), &plist);

            if (plist == nullptr) {
              throw Exception(ADSError::ServiceError, "Invalid plist response");
            }

            // Error process
            auto nrc = plist_dict_get_item(plist, "resultCode");
            if (nrc) {
              uint64_t code = 0;
              plist_get_uint_val(nrc, &code);
              if (code != 0) {
                auto nrs = plist_dict_get_item(plist, "resultString");
                if (nrs) {
                  char *str = nullptr;
                  plist_get_string_val(nrs, &str);
                  std::string msg = str;
                  free(str);

                  plist_free(plist);
                  throw Exception(ADSError::ServiceError, msg);
                }
              }
            }
            return plist;
          });

  free(plistXML);
  plist_free(plist);

  return task;
}

} // namespace iris