#include "anisette.h"
#include "authutils.h"
#include "csr.h"
#include "service_client.h"

namespace iris {
using namespace utility;           // Common utilities like string conversions
using namespace web;               // Common features like URIs.
using namespace web::http;         // Common HTTP functionality
using namespace web::http::client; // HTTP client features
using namespace concurrency::streams; // Asynchronous streams

pplx::task<provision_profile> ServiceClientQH65B2::fetch_provisioning_profile(
    const std::string &app_id, const device_type::type &dty,
    const std::string &team_id, const std::string &adsid,
    const std::string &auth_token) {

  std::map<std::string, plist_t> parameters = {
      {"appIdId", plist_new_string(app_id.c_str())}};

  switch (dty) {
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

  return this
      ->send_request("ios/downloadTeamProvisioningProfile.action", team_id,
                     adsid, auth_token, parameters)
      .then([](plist_t plist) {
        provision_profile profile;

        auto np = plist_dict_get_item(plist, "provisioningProfile");
        if (np) {
          auto pid = plist_dict_get_item(np, "provisioningProfileId");
          GET_NODE_STR(profile.id, pid);
          profile.__isset.id = true;
          auto name = plist_dict_get_item(np, "name");
          GET_NODE_STR(profile.name, name);
          auto type = plist_dict_get_item(np, "type");     // iOS Development
          GET_NODE_STR(profile.type, type);
          
          auto status = plist_dict_get_item(np, "status"); // Active
          GET_NODE_STR(profile.status, status);

          auto managed_app = plist_dict_get_item(np, "managingApp"); // Xcode

          auto app = plist_dict_get_item(np, "appId"); // AppId entitlements
          if (app) {
            auto bid = plist_dict_get_item(app, "identifier");
            GET_NODE_STR(profile.bundle_id, bid);
            auto tid = plist_dict_get_item(app, "prefix");
            GET_NODE_STR(profile.team_id, tid);

            auto features = plist_dict_get_item(app, "features"); // TODO
          }

          auto appidid =
              plist_dict_get_item(np, "appIdId"); // AppId of Bundle_ID
          auto prof = plist_dict_get_item(
              np, "encodedProfile"); // bytes, used in signer
          if (prof) {
            char *bytes = nullptr;
            uint64_t length = 0;
            plist_get_data_val(prof, &bytes, &length);
            profile.data.resize(length);
            memcpy(profile.data.data(), bytes, length);
            free(bytes);
          }
        }

        plist_free(plist);
        return profile;
      });
}

pplx::task<bool> ServiceClientQH65B2::delete_provisioning_profile(
    const std::string &pp_id, const std::string &team_id,
    const std::string &adsid, const std::string &auth_token) {

  std::map<std::string, plist_t> parameters = {
      {"provisioningProfileId", plist_new_string(pp_id.c_str())},
      {"teamId", plist_new_string(team_id.c_str())},
  };

  return this
      ->send_request("ios/deleteProvisioningProfile.action", team_id, adsid,
                     auth_token, parameters)
      .then([](plist_t plist) {
        // auto node = plist_dict_get_item(plist, "resultCode");
        // if (node == nullptr) {
        //  throw APIError(APIErrorCode::InvalidResponse);
        //}

        // uint64_t resultCode = 0;
        // plist_get_uint_val(node, &resultCode);

        // if (resultCode != 0) {
        //  // Need to throw an exception for resultCodeHandler to be called.
        //  throw APIError(APIErrorCode::InvalidProvisioningProfileIdentifier);
        //}
        /*switch (resultCode) {
          case 35:
            return std::make_optional<APIError>(
                APIErrorCode::InvalidProvisioningProfileIdentifier);

          case 8101:
            return std::make_optional<APIError>(
                APIErrorCode::ProvisioningProfileDoesNotExist);

          default:
            return std::nullopt;
          }*/
        plist_free(plist);
        return true;
      });
}

} // namespace iris