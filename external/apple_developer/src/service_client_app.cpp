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
using namespace std;

pplx::task<std::vector<app_info>>
ServiceClientQH65B2::fetch_app_ids(const std::string &team_id,
                                   const std::string &adsid,
                                   const std::string &auth_token) {
  return this
      ->send_request("ios/listAppIds.action", team_id, adsid, auth_token, {})
      .then([](plist_t plist) {
        vector<app_info> apps;

        auto napps = plist_dict_get_item(plist, "appIds");
        if (napps)
        {
          int n = plist_array_get_size(napps);
          for (int i = 0; i < n; i++) {
            app_info info;
            auto item = plist_array_get_item(napps, i);
            auto nname = plist_dict_get_item(item, "name");
            GET_NODE_STR(info.name, nname);
            auto nbid = plist_dict_get_item(item, "identifier");
            GET_NODE_STR(info.bundle_id, nbid);
            auto nid = plist_dict_get_item(item, "appIdId");
            GET_NODE_STR(info.id, nid);
            // TODO
            auto nfeatures = plist_dict_get_item(item, "features");
            auto nefeatures = plist_dict_get_item(item, "enabledFeatures");

            apps.push_back(info);
          }
        }
        return apps;
      });
}

pplx::task<app_info> ServiceClientQH65B2::add_app_id(
    const std::string &name, const std::string &bundle_id,
    const std::string &team_id, const std::string &adsid,
    const std::string &auth_token) {
  std::map<std::string, plist_t> parameters = {
      {"name", plist_new_string(name.c_str())},
      {"identifier", plist_new_string(bundle_id.c_str())},
  };
  return this
      ->send_request("ios/addAppId.action", team_id, adsid, auth_token,
                     parameters)
      .then([](plist_t plist) {
        app_info app;
        // TODO
        return app;
      });
}
pplx::task<app_info> ServiceClientQH65B2::update_app_id(
    const std::string &app_id, const std::string &team_id,
    const std::string &adsid, const std::string &auth_token) {
  std::map<std::string, plist_t> parameters = {
      {"appIdId", plist_new_string(app_id.c_str())}
      // TODO: features
  };
  return this
      ->send_request("ios/updateAppId.action", team_id, adsid, auth_token,
                     parameters)
      .then([](plist_t plist) {
        app_info app;
        // TODO
        return app;
      });
}

pplx::task<std::vector<app_group_info>>
ServiceClientQH65B2::fetch_app_groups(const std::string &team_id,
                                      const std::string &adsid,
                                      const std::string &auth_token) {
  return this
      ->send_request("ios/listApplicationGroups.action", team_id, adsid,
                     auth_token, {})
      .then([](plist_t plist) {
        vector<app_group_info> app_groups;
        
        auto nagps = plist_dict_get_item(plist, "applicationGroupList");
        if (nagps) {
          int num_agps = plist_array_get_size(nagps);
          for (int i = 0; i < num_agps; i++) {
            app_group_info agi;
            auto nagp = plist_array_get_item(nagps, i);
            auto gid = plist_dict_get_item(nagp, "applicationGroup");
            GET_NODE_STR(agi.group_id, gid);
            auto nname = plist_dict_get_item(nagp, "name");
            GET_NODE_STR(agi.name, nname);
            plist_dict_get_item(nagp, "prefix");
            auto id = plist_dict_get_item(nagp, "identifier");
            GET_NODE_STR(agi.id, id);
            plist_dict_get_item(nagp, "status");
            app_groups.push_back(agi);
          }
        }

        plist_free(plist);
        return app_groups;
      });
}
pplx::task<app_group_info> ServiceClientQH65B2::add_app_group(
    const std::string &name, const std::string &group_id,
    const std::string &team_id, const std::string &adsid,
    const std::string &auth_token) {
  return pplx::task<app_group_info>();
}
pplx::task<bool> ServiceClientQH65B2::assign_app_to_groups(
    const std::string &app_id, const std::vector<std::string> &groups,
    const std::string &team_id, const std::string &adsid,
    const std::string &auth_token) {
  return pplx::task<bool>();
}
} // namespace iris