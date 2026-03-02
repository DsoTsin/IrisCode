#pragma once

#include "developer.h"

namespace iris {
class GsaClient;
class ServiceClient;
class ServiceClientQH65B2;

class DeveloperHandler : public developerIf {
public:
  DeveloperHandler(apache::thrift::TConnectionInfo const& conn);
  ~DeveloperHandler();

  void login(login_response &_return, const std::string &appleid,
             const std::string &password) override;
  void verify(verify_response &_return, const std::string &code) override;
  virtual void fetch_team(std::vector<developer_team> &_return) override;
  virtual void fetch_devices(std::vector<device> &_return,
                             const std::string &team) override;
  virtual void register_device(const device &dev,
                               const std::string &team) override;
  virtual void fetch_certificates(std::vector<certificate> &_return,
                                  const std::string &team) override;
  virtual void add_certificate(certificate &_return,
                               const std::string &machine_name,
                               const std::string &team) override;
  virtual bool revoke_certificate(const std::string &cert_id,
                                  const std::string &team) override;
  virtual void fetch_apps(std::vector<app_info> &_return,
                          const std::string &team) override;
  virtual void add_app(app_info &_return, const std::string &name,
                       const std::string &bundle_id,
                       const std::string &team) override;
  virtual void update_app(app_info &_return, const app_info &app,
                          const std::string &team) override;
  virtual void fetch_app_groups(std::vector<app_group_info> &_return,
                                const std::string &team) override;
  virtual void add_app_group(app_group_info &_return,
                             const std::string &group_name,
                             const std::string &group_id,
                             const std::string &team) override;
  virtual bool assign_app_group(const app_info &app,
                                const std::vector<app_group_info> &groups,
                                const std::string &team) override;
  virtual void fetch_provisioning_profiles(provision_profile &_return,
                                           const std::string &app_id,
                                           const device_type::type dty,
                                           const std::string &team) override;
  virtual bool delete_provisioning_profile(const std::string &pp_id,
                                           const std::string &team) override;

private:
  apache::thrift::TConnectionInfo conn_;

  std::string adsid_; // required
  std::string idms_token_;

  std::string auth_token_;

  std::shared_ptr<GsaClient> gsaclient_;
  std::shared_ptr<ServiceClient> bsclient_;
  std::shared_ptr<ServiceClientQH65B2> sclient_;
};
} // namespace iris