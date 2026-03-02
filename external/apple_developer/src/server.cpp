#include "server.h"
#include "apple_developer_service_api.h"
#include "developer_api.h"
#include "gsa_client.h"
#include "service_client.h"

#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TPipeServer.h>

namespace iris {
using namespace std;
DeveloperHandler::DeveloperHandler(apache::thrift::TConnectionInfo const &conn)
    : conn_(conn), gsaclient_(make_shared<GsaClient>()),
      bsclient_(make_shared<ServiceClient>()),
      sclient_(make_shared<ServiceClientQH65B2>()) {}

DeveloperHandler::~DeveloperHandler() {}

void DeveloperHandler::login(login_response &rsp, const std::string &appleid,
                             const std::string &password) {
  //plist_t result = nullptr;
  auto result = gsaclient_->authenticate(appleid, password).get();
  if (result) {
    auto adsidNode = plist_dict_get_item(result, "adsid");
    auto idmsTokenNode = plist_dict_get_item(result, "GsIdmsToken");
    char *adsid = nullptr;
    plist_get_string_val(adsidNode, &adsid);
    char *idmsToken = nullptr;
    plist_get_string_val(idmsTokenNode, &idmsToken);
    adsid_ = adsid;
    idms_token_ = idmsToken;
    free(adsid);
    free(idmsToken);
    auto two_factor = plist_dict_get_item(result, "twoFactorEnabled");
    rsp.need_2factor_code = two_factor ? true : false;

    if (two_factor) {
      verify_response rsp;
      developerClient client(conn_.input, conn_.output);
      client.send_verify("request_verify");
      client.recv_verify(rsp);
      auto _bytes = gsaclient_->verify(rsp.code, adsid_, idms_token_).get();
      plist_free(result);
      result = gsaclient_->authenticate(appleid, password).get();
      if (result) {
        auto adsidNode = plist_dict_get_item(result, "adsid");
        auto idmsTokenNode = plist_dict_get_item(result, "GsIdmsToken");
        char *adsid = nullptr;
        plist_get_string_val(adsidNode, &adsid);
        char *idmsToken = nullptr;
        plist_get_string_val(idmsTokenNode, &idmsToken);
        adsid_ = adsid;
        idms_token_ = idmsToken;
        free(adsid);
        free(idmsToken);

        auto skNode = plist_dict_get_item(result, "sk");
        auto cNode = plist_dict_get_item(result, "c");

        if (skNode == nullptr || cNode == nullptr) {
          odslog("ERROR: No ak and /or c data.");
          // throw APIError(APIErrorCode::InvalidResponse);
        }

        char *skBytes = nullptr;
        uint64_t skSize = 0;
        plist_get_data_val(skNode, &skBytes, &skSize);

        char *cBytes = nullptr;
        uint64_t cSize = 0;
        plist_get_data_val(cNode, &cBytes, &cSize);

        // sk, c, adsid, idmsToken -> auth_token
        auth_token_ =
            gsaclient_
                ->fetch_auth_token(bytes_view((const uint8_t *)skBytes, skSize),
                                   bytes_view((const uint8_t *)cBytes, cSize),
                                   adsid_, idms_token_)
                .get();

        free(skBytes);
        free(cBytes);

        auto account = sclient_->fetch_account(adsid_, auth_token_).get();
        if (account.has_value()) {
          rsp.__set_account(*account);
        }
      }

    } else { // no two factor
      auto skNode = plist_dict_get_item(result, "sk");
      auto cNode = plist_dict_get_item(result, "c");

      if (skNode == nullptr || cNode == nullptr) {
        odslog("ERROR: No ak and /or c data.");
        // throw APIError(APIErrorCode::InvalidResponse);
      }

      char *skBytes = nullptr;
      uint64_t skSize = 0;
      plist_get_data_val(skNode, &skBytes, &skSize);

      char *cBytes = nullptr;
      uint64_t cSize = 0;
      plist_get_data_val(cNode, &cBytes, &cSize);

      // sk, c, adsid, idmsToken -> auth_token
      auth_token_ =
          gsaclient_
              ->fetch_auth_token(bytes_view((const uint8_t *)skBytes, skSize),
                                 bytes_view((const uint8_t *)cBytes, cSize),
                                 adsid_, idms_token_)
              .get();

      free(skBytes);
      free(cBytes);

      auto account = sclient_->fetch_account(adsid_, auth_token_).get();
      if (account.has_value()) {
        rsp.__set_account(*account);
      }
    }
    plist_free(result);
  }
}
void DeveloperHandler::verify(verify_response &rsp, const std::string &code) {
  auto result = gsaclient_->verify(code, adsid_, idms_token_).get();
  // TODO: reauthenticate??
}
void DeveloperHandler::fetch_team(std::vector<developer_team> &teams) {
  teams = move(sclient_->fetch_teams(adsid_, auth_token_).get());
}
void DeveloperHandler::fetch_devices(std::vector<device> &devs,
                                     const std::string &team) {
  devs = move(sclient_->fetch_devices(team, adsid_, auth_token_).get());
}
void DeveloperHandler::register_device(const device &dev,
                                       const std::string &team) {
  sclient_->register_device(dev.name, dev.id, dev.type, team, adsid_,
                            auth_token_); // TODO: return result handle..
}
void DeveloperHandler::fetch_certificates(std::vector<certificate> &cert,
                                          const std::string &team) {
  cert = std::move(
      bsclient_->fetch_development_certificates(team, adsid_, auth_token_)
          .get());
}
void DeveloperHandler::add_certificate(certificate &cert,
                                       const std::string &machine_name,
                                       const std::string &team) {
  // TODO: machine serial as param ?
  cert = std::move(
      sclient_
          ->add_development_certificate(machine_name, team, adsid_, auth_token_)
          .get());
}
bool DeveloperHandler::revoke_certificate(const std::string &cert_id,
                                          const std::string &team) {
  return bsclient_->revoke_certificate(cert_id, team, adsid_, auth_token_)
      .get();
}
void DeveloperHandler::fetch_apps(std::vector<app_info> &apps,
                                  const std::string &team) {
  apps = std::move(sclient_->fetch_app_ids(team, adsid_, auth_token_).get());
}
void DeveloperHandler::add_app(app_info &app, const std::string &name,
                               const std::string &bundle_id,
                               const std::string &team) {
  app = std::move(
      sclient_->add_app_id(name, bundle_id, team, adsid_, auth_token_).get());
}
void DeveloperHandler::update_app(app_info &out_app, const app_info &app,
                                  const std::string &team) {}

void DeveloperHandler::fetch_app_groups(std::vector<app_group_info> &app_groups,
                                        const std::string &team) {
  app_groups =
      std::move(sclient_->fetch_app_groups(team, adsid_, auth_token_).get());
}
void DeveloperHandler::add_app_group(app_group_info &app_group,
                                     const std::string &group_name,
                                     const std::string &group_id,
                                     const std::string &team) {
  app_group = std::move(
      sclient_->add_app_group(group_name, group_id, team, adsid_, auth_token_)
          .get());
}
// TODO
bool DeveloperHandler::assign_app_group(
    const app_info &app, const std::vector<app_group_info> &groups,
    const std::string &team) {

  // sclient_->assign_app_to_groups()

  return false;
}
void DeveloperHandler::fetch_provisioning_profiles(provision_profile &pp,
                                                   const std::string &app_id,
                                                   const device_type::type dty,
                                                   const std::string &team) {
  pp = move(
      sclient_
          ->fetch_provisioning_profile(app_id, dty, team, adsid_, auth_token_)
          .get());
}
bool DeveloperHandler::delete_provisioning_profile(const std::string &pp_id,
                                                   const std::string &team) {
  return sclient_->delete_provisioning_profile(pp_id, team, adsid_, auth_token_)
      .get();
}

class DeveloperAPIFactory : virtual public developerIfFactory {
public:
  explicit DeveloperAPIFactory();
  ~DeveloperAPIFactory() override;
  developerIf *
  getHandler(const apache::thrift::TConnectionInfo &connInfo) override;
  void releaseHandler(developerIf *handler) override;

public:
private:
  friend class DeveloperHandler;
};

class DeveloperAPIServer : public ::apache::thrift::server::TThreadedServer {
private:
protected:
  void onClientConnected(
      const shared_ptr<apache::thrift::server::TConnectedClient> &client)
      override;
  void onClientDisconnected(
      apache::thrift::server::TConnectedClient *client) override;

public:
  DeveloperAPIServer();
  ~DeveloperAPIServer() override;
};

} // namespace iris

int main(int argc, const char *argv[]) {
  iris::DeveloperAPIServer server;
  server.serve();
  return 0;
}

namespace iris {
using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

DeveloperAPIFactory::DeveloperAPIFactory() {}
DeveloperAPIFactory::~DeveloperAPIFactory() {}

developerIf *DeveloperAPIFactory::getHandler(
    const apache::thrift::TConnectionInfo &connInfo) {
  return new DeveloperHandler(connInfo);
}
void DeveloperAPIFactory::releaseHandler(developerIf *handler) {
  delete handler;
}

DeveloperAPIServer::DeveloperAPIServer()
    : TThreadedServer(make_shared<developerProcessorFactory>(
                          make_shared<DeveloperAPIFactory>()),
                      static_pointer_cast<TServerTransport>(
                          make_shared<TPipeServer>(API_ENDPOINT)),
                      make_shared<TBufferedTransportFactory>(),
                      make_shared<TBinaryProtocolFactory>()) {}

DeveloperAPIServer::~DeveloperAPIServer() {}

void DeveloperAPIServer::onClientConnected(
    const shared_ptr<apache::thrift::server::TConnectedClient> &client) {
  TThreadedServer::onClientConnected(client);
}

void DeveloperAPIServer::onClientDisconnected(
    apache::thrift::server::TConnectedClient *client) {
  TThreadedServer::onClientDisconnected(client);
}
} // namespace iris