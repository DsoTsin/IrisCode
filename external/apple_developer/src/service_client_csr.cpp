#include "anisette.h"
#include "authutils.h"
#include "csr.h"
#include "service_client.h"

extern "C" unsigned char *base64decode(const char *buf, size_t *size);

namespace iris {
using namespace utility;           // Common utilities like string conversions
using namespace web;               // Common features like URIs.
using namespace web::http;         // Common HTTP functionality
using namespace web::http::client; // HTTP client features
using namespace concurrency::streams; // Asynchronous streams

// Certificates
pplx::task<std::vector<certificate>>
ServiceClient::fetch_development_certificates(
    const std::string &team_id, const std::string &dsid,
    const std::string &auth_token) {
#if 1
  std::map<std::string, std::string> params = {
      {"filter[certificateType]", "IOS_DEVELOPMENT"},
  };
  return ServiceClient::send_request("certificates", "GET", team_id, dsid,
                                     auth_token, params)
      .then([](json::value json) {
        std::vector<certificate> result;
        if (json.has_array_field(L"data")) {
          auto data = json[L"data"].as_array();
          for (auto& elem : data) {
            certificate cert;
            cert.id = to_utf8(elem[L"id"].as_string());
            auto attributes = elem[L"attributes"];
            if (attributes.has_field(L"certificateContent")) {
              auto encodedData = to_utf8(attributes[L"certificateContent"].as_string());
              size_t data_len = encodedData.length();
              auto decodedData = base64decode(encodedData.c_str(), &data_len);
              cert.data.resize(data_len);
              memcpy(cert.data.data(), decodedData, data_len);
              cert.__isset.data = 1;
            }

            cert.machine_name = to_utf8(attributes[L"machineName"].as_string());
            cert.machine_id = to_utf8(attributes[L"machineId"].as_string());
            cert.name = to_utf8(attributes[L"name"].as_string());
            cert.serial_no = to_utf8(attributes[L"serialNumber"].as_string());
            result.push_back(cert);
          }
        }
        return result;
      });
#else
  return this
      ->send_request("ios/certificate/downloadCertificateContent.action", team_id, dsid,
                     auth_token, {})
      .then([](plist_t plist) {
        std::vector<certificate> result;
        // TODO
        return result;
      });
#endif
}

pplx::task<certificate> ServiceClientQH65B2::add_development_certificate(
    const std::string &machine_name, const std::string &team_id,
    const std::string &dsid, const std::string &auth_token) {
  CodesignRequest csr;
  auto uuid = make_uuid();
  std::map<std::string, plist_t> parameters = {
      {"csrContent", plist_new_string(csr.data().c_str())},
      {"machineId", plist_new_string(uuid.c_str())},
      {"machineName", plist_new_string(machine_name.c_str())}};
  return this
      ->send_request("ios/submitDevelopmentCSR.action", team_id, dsid,
                     auth_token, parameters)
      .then([pkey = csr.privateKey()](plist_t plist) {
        certificate cert;
        auto node = plist_dict_get_item(plist, "certRequest");
        if (node == nullptr) {
          plist_free(plist);
          throw Exception(ADSError::ServiceError, "certRequest failed!");
        }

        auto status = plist_dict_get_item(node, "statusString");
        std::string str_status;
        GET_NODE_STR(str_status, status);
        if (str_status != "Approved") {
          plist_free(plist);
          throw Exception(ADSError::ServiceError,
                                 "certRequest not approved!");
        }

        auto machineNameNode = plist_dict_get_item(node, "machineName");
        GET_NODE_STR(cert.machine_name, machineNameNode);
        auto machineIdentifierNode = plist_dict_get_item(node, "machineId");
        GET_NODE_STR(cert.machine_id, machineIdentifierNode);

        auto ncertid = plist_dict_get_item(node, "certificateId");
        GET_NODE_STR(cert.id, ncertid);

        auto nserial = plist_dict_get_item(node, "serialNum");
        if (nserial) {
          GET_NODE_STR(cert.serial_no, nserial);
        }

        // Important
        cert.__set_priv_key(pkey);

        plist_free(plist);
        return cert;
      });
}

pplx::task<bool> ServiceClient::revoke_certificate(
    const std::string &cert_id, const std::string &team_id,
    const std::string &dsid, const std::string &auth_token) {

  std::ostringstream ss;
  ss << "certificates/" << cert_id;
  return ServiceClient::send_request(ss.str(), "DELETE", team_id, dsid,
                                     auth_token, {})
      .then([](json::value json) {
        // TODO
        return true;
      });
}
} // namespace iris