#pragma once

#include "exception.h"
#include "gsa_client.h"

namespace iris {
class ServiceClient : public web::http::client::http_client {
public:
  ServiceClient();
  virtual ~ServiceClient();

  // Only used in cert delete(revoke) and fetch
  pplx::task<web::json::value>
  send_request(const std::string &uri, const std::string &method,
               const std::string &team_id, const std::string &dsid,
               const std::string &auth_token,
               const std::map<std::string, std::string> &parameters);

  // Certificates
  pplx::task<std::vector<certificate>>
  fetch_development_certificates(const std::string &team_id,
                                 const std::string &adsid,
                                 const std::string &auth_token);

  pplx::task<bool> revoke_certificate(const std::string &cert_id,
                                      const std::string &team_id,
                                      const std::string &adsid,
                                      const std::string &auth_token);

protected:
  ServiceClient(const web::uri &);
};
class ServiceClientQH65B2 : public ServiceClient {
public:
  ServiceClientQH65B2();
  ~ServiceClientQH65B2() override;

  pplx::task<std::optional<developer_account>>
  fetch_account(const std::string &adsid, const std::string &auth_token);

  // Teams
  pplx::task<std::vector<developer_team>>
  fetch_teams(const std::string &adsid, const std::string &auth_token);

  // Devices
  pplx::task<std::vector<device>> fetch_devices(const std::string &team_id,
                                                const std::string &adsid,
                                                const std::string &auth_token);
  void register_device(const std::string &dev_name,
                       const std::string &dev_identifier,
                       const device_type::type &dev_type,
                       const std::string &team_id, const std::string &adsid,
                       const std::string &auth_token);


  pplx::task<certificate> add_development_certificate(
      const std::string &machine_name, const std::string &team_id,
      const std::string &adsid, const std::string &auth_token);

  // App
  pplx::task<std::vector<app_info>>
  fetch_app_ids(const std::string &team_id, const std::string &adsid,
                const std::string &auth_token);

  pplx::task<app_info> add_app_id(const std::string &name,
                                  const std::string &bundle_id,
                                  const std::string &team_id,
                                  const std::string &adsid,
                                  const std::string &auth_token);
  // TODO: features, entitlement
  pplx::task<app_info>
  update_app_id(const std::string &app_id, /*const std::map<> &features,*/
                const std::string &team_id, const std::string &adsid,
                const std::string &auth_token);

  // App Groups
  pplx::task<std::vector<app_group_info>>
  fetch_app_groups(const std::string &team_id, const std::string &adsid,
                   const std::string &auth_token);
  pplx::task<app_group_info> add_app_group(const std::string &name,
                                           const std::string &group_id,
                                           const std::string &team_id,
                                           const std::string &adsid,
                                           const std::string &auth_token);
  pplx::task<bool> assign_app_to_groups(const std::string &app_id,
                                        const std::vector<std::string> &groups,
                                        const std::string &team_id,
                                        const std::string &adsid,
                                        const std::string &auth_token);

  // Provisioning Profiles
  pplx::task<provision_profile> fetch_provisioning_profile(
      const std::string &app_id, const device_type::type &dty,
      const std::string &team_id, const std::string &adsid,
      const std::string &auth_token);

  pplx::task<bool> delete_provisioning_profile(const std::string &pp_id,
                                               const std::string &team_id,
                                               const std::string &adsid,
                                               const std::string &auth_token);

protected:
  pplx::task<plist_t>
  send_request(const std::string &uri, const std::string &team_id,
               const std::string &adsid, const std::string &auth_token,
               // const std::map<std::string, plist_t> &parameters,
               const std::map<std::string, plist_t> &additional_params);
};

#define GET_NODE_STR(value, node)                                              \
  {                                                                            \
    char *val = nullptr;                                                       \
    plist_get_string_val(node, &val);                                          \
    value = val;                                                               \
    free(val);                                                                 \
  }

} // namespace iris