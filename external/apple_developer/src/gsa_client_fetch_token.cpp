#include "anisette.h"
#include "authutils.h"
#include "gsa_client.h"

namespace iris {
using namespace utility;           // Common utilities like string conversions
using namespace web;               // Common features like URIs.
using namespace web::http;         // Common HTTP functionality
using namespace web::http::client; // HTTP client features
using namespace concurrency::streams; // Asynchronous streams

std::string kAppName = "com.apple.gs.xcode.auth";

pplx::task<std::string>
GsaClient::fetch_auth_token(const bytes_view &sk, const bytes_view &c,
                            const std::string &adsid,
                            const std::string &idms_token) {
  auto checksum = ALTCreateAppTokensChecksum(sk, adsid, {kAppName});

  auto appsNode = plist_new_array();
  plist_array_append_item(appsNode, plist_new_string(kAppName.c_str()));
  auto header_dict = Anisette::get().make_dict();
  auto cpd = plist_new_dict();
  for (auto &pair : header_dict) {
    plist_dict_set_item(cpd, pair.first.c_str(), pair.second);
  }

  std::map<std::string, plist_t> parameters = {
      {"u", plist_new_string(adsid.c_str())},
      {"app", appsNode},
      {"c", plist_new_data((const char *)c.data(), c.size())},
      {"t", plist_new_string(idms_token.c_str())},
      {"checksum",
       plist_new_data((const char *)checksum.data(), checksum.size())},
      {"cpd", cpd},
      {"o", plist_new_string("apptokens")}};

  return send_auth(parameters).then([sk = sk.clone()](plist_t plist) {
    auto encryptedTokenNode = plist_dict_get_item(plist, "et");
    if (encryptedTokenNode == nullptr) {
      plist_free(plist);
      throw Exception(ADSError::InvalidAuthenticateResponse, "Missing et!");
    }

    char *encryptedTokenBytes = nullptr;
    uint64_t encryptedTokenSize = 0;
    plist_get_data_val(encryptedTokenNode, &encryptedTokenBytes,
                       &encryptedTokenSize);

    auto sk_copy = sk;
    auto decryptedToken = ALTDecryptDataGCM(
        sk_copy,
        bytes_view((const uint8_t *)encryptedTokenBytes, encryptedTokenSize));
    free(encryptedTokenBytes);

    if (decryptedToken == std::nullopt) {
      odslog("ERROR: Failed to decrypt apptoken.");
      plist_free(plist);
      throw Exception(ADSError::InvalidAuthenticateResponse,
                      "Failed to decrypt apptoken!");
    }

    plist_t decryptedTokenPlist = nullptr;
    plist_from_xml((const char *)decryptedToken->data(), decryptedToken->size(),
                   &decryptedTokenPlist);

    if (decryptedTokenPlist == nullptr) {
      odslog("ERROR: Could not parse decrypted apptoken plist.");
      plist_free(plist);
      throw Exception(ADSError::InvalidAuthenticateResponse,
                      "Could not parse decrypted apptoken plist.");
    }

    auto tokensNode = plist_dict_get_item(decryptedTokenPlist, "t");
    if (tokensNode == nullptr) {
      plist_free(decryptedTokenPlist);
      plist_free(plist);
      throw Exception(ADSError::InvalidAuthenticateResponse, "Missing t!");
    }

    auto tokenDictionary = plist_dict_get_item(tokensNode, kAppName.c_str());
    if (tokenDictionary == nullptr) {
      plist_free(decryptedTokenPlist);
      plist_free(plist);
      throw Exception(ADSError::InvalidAuthenticateResponse, "Missing t!");
    }

    auto tokenNode = plist_dict_get_item(tokenDictionary, "token");
    if (tokenNode == nullptr) {
      plist_free(decryptedTokenPlist);
      plist_free(plist);
      throw Exception(ADSError::InvalidAuthenticateResponse, "Missing token!");
    }

    char *token = nullptr;
    plist_get_string_val(tokenNode, &token);
    auto strToken = std::string(token);
    odslog("Got token for " << kAppName << "!\nValue : " << token);
    free(token);

    plist_free(decryptedTokenPlist);
    plist_free(plist);
    return strToken;
  });
}

} // namespace iris