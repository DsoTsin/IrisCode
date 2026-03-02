#include "anisette.h"
#include "authutils.h"
#include "gsa_client.h"
#include "exception.h"

namespace iris {
using namespace utility;           // Common utilities like string conversions
using namespace web;               // Common features like URIs.
using namespace web::http;         // Common HTTP functionality
using namespace web::http::client; // HTTP client features
using namespace concurrency::streams; // Asynchronous streams

pplx::task<plist_t> GsaClient::authenticate(const std::string &account,
                                            const std::string &password) {
  Anisette::get().provision();
  auto header_dict = Anisette::get().make_dict();
  header_dict.insert({"bootstrap", plist_new_bool(true)});
  header_dict.insert({"icscrec", plist_new_bool(true)});
  header_dict.insert({"pbe", plist_new_bool(false)});
  header_dict.insert({"prkgen", plist_new_bool(true)});
  header_dict.insert({"svct", plist_new_string("iCloud")});
  header_dict.insert(
      {"loc", plist_new_string(Anisette::get().locale().c_str())});
  auto altd = new ALTDigest;
  std::vector<std::string> ps = {"s2k", "s2k_fo"};
  altd->update_string(ps[0]);
  altd->update_string(",");
  altd->update_string(ps[1]);
  auto A_bytes = altd->start_client_auth();
  altd->update_string("|");
  auto psPlist = plist_new_array();
  for (auto value : ps) {
    plist_array_append_item(psPlist, plist_new_string(value.c_str()));
  }

  auto cpd = plist_new_dict();
  for (auto &pair : header_dict) {
    plist_dict_set_item(cpd, pair.first.c_str(), pair.second);
  }

  std::map<std::string, plist_t> parameters = {
      {"A2k", plist_new_data((const char *)A_bytes.data(), A_bytes.size())},
      {"ps", psPlist},
      {"cpd", cpd},
      {"u", plist_new_string(account.c_str())},
      {"o", plist_new_string("init")}};

  auto complete = [=](plist_t plist) {
    auto spNode = plist_dict_get_item(plist, "sp");
    if (spNode == nullptr) {
      plist_free(plist);
      throw Exception(ADSError::InvalidAuthenticateResponse,
                         "sp not found!");
    }

    char *sp = nullptr;
    plist_get_string_val(spNode, &sp);

    bool isS2K = (std::string(sp) == "s2k");
    altd->update_string("|");
    if (sp) {
      altd->update_string(sp);
    }
    auto cNode = plist_dict_get_item(plist, "c");
    auto saltNode = plist_dict_get_item(plist, "s");
    auto iterationsNode = plist_dict_get_item(plist, "i");
    auto bNode = plist_dict_get_item(plist, "B");

    if (cNode == nullptr || saltNode == nullptr || iterationsNode == nullptr ||
        bNode == nullptr) {
      plist_free(plist);
      throw Exception(ADSError::InvalidAuthenticateResponse,
                         "c salt iteration b not found!");
    }

    char *c = nullptr;
    plist_get_string_val(cNode, &c);

    char *saltBytes = nullptr;
    uint64_t saltSize = 0;
    plist_get_data_val(saltNode, &saltBytes, &saltSize);

    uint64_t iterations = 0;
    plist_get_uint_val(iterationsNode, &iterations);

    char *B_bytes = nullptr;
    uint64_t B_size = 0;
    plist_get_data_val(bNode, &B_bytes, &B_size);

    auto salt = bytes(saltBytes, saltBytes + saltSize);
    auto B_data = bytes(B_bytes, B_bytes + B_size);
    bytes M_bytes;
    altd->client_process_challege(account, password, salt, B_data, iterations,
                                  isS2K, M_bytes);
    auto header_dict = Anisette::get().make_dict();
    header_dict.insert({"bootstrap", plist_new_bool(true)});
    header_dict.insert({"icscrec", plist_new_bool(true)});
    header_dict.insert({"pbe", plist_new_bool(false)});
    header_dict.insert({"prkgen", plist_new_bool(true)});
    header_dict.insert({"svct", plist_new_string("iCloud")});
    header_dict.insert(
        {"loc", plist_new_string(Anisette::get().locale().c_str())});

    auto cpd = plist_new_dict();
    for (auto &pair : header_dict) {
      plist_dict_set_item(cpd, pair.first.c_str(), pair.second);
    }

    std::map<std::string, plist_t> parameters = {
        {"c", plist_new_string(c)},
        {"M1", plist_new_data((const char *)M_bytes.data(), M_bytes.size())},
        {"cpd", cpd},
        {"u", plist_new_string(account.c_str())},
        {"o", plist_new_string("complete")}};
    return send_auth(parameters);
  };

  auto auth_handler = [=](plist_t plist) {
    auto M2_node = plist_dict_get_item(plist, "M2");
    if (M2_node == nullptr) {
      odslog("ERROR: M2 data not found!");
      plist_free(plist);
      throw Exception(ADSError::InvalidAuthenticateResponse,
                         "M2 data not found!");
    }

    char *M2_bytes = nullptr;
    uint64_t M2_size = 0;
    plist_get_data_val(M2_node, &M2_bytes, &M2_size);
    if (!altd->verify_session(bytes_view((const uint8_t *)M2_bytes, M2_size))) {
      odslog("ERROR: Failed to verify session.");

      free(M2_bytes);

      plist_free(plist);
      throw Exception(ADSError::InvalidAuthenticateResponse,
                         "Failed to verify session.");
    }
    free(M2_bytes);

    altd->update_string("|");
    bytes spd;
    auto spdNode = plist_dict_get_item(plist, "spd");
    if (spdNode != nullptr) {
      char *spdBytes = nullptr;
      uint64_t spdSize = 0;
      plist_get_data_val(spdNode, &spdBytes, &spdSize);
      auto spd_view = bytes_view((const uint8_t *)spdBytes, spdSize);
      spd = spd_view.clone();

      free(spdBytes);

      altd->update_data(spd_view);
    }

    altd->update_string("|");
    auto scNode = plist_dict_get_item(plist, "sc");
    if (scNode != nullptr) {
      char *scBytes = nullptr;
      uint64_t scSize = 0;
      plist_get_data_val(scNode, &scBytes, &scSize);
      altd->update_data(bytes_view((const uint8_t *)scBytes, scSize));
    }
    altd->update_string("|");
    auto npNode = plist_dict_get_item(plist, "np");
    if (npNode == nullptr) {
      odslog("ERROR: Missing np dictionary.");
      plist_free(plist);
      throw Exception(ADSError::InvalidAuthenticateResponse,
                         "Missing np dictionary!");
    }

    char *npBytes = nullptr;
    uint64_t npSize = 0;
    plist_get_data_val(npNode, &npBytes, &npSize);
    altd->update_data(bytes_view((const uint8_t *)npBytes, npSize));
    free(npBytes);

    bytes digest;
    altd->final_digest(digest);
    auto decryptedData = altd->decrypt_cbc(spd);

    odslog("Data: " << decryptedData->data());
    #ifdef _DEBUG
    std::string resp = std::string((*decryptedData).begin(), (*decryptedData).end());
    #endif

    plist_t decryptedPlist = nullptr;
    plist_from_xml((const char *)decryptedData->data(),
                   (int)decryptedData->size(), &decryptedPlist);

    if (decryptedPlist == nullptr) {
      odslog("ERROR: Could not parse decrypted login response plist!");
      plist_free(plist);
      throw Exception(ADSError::InvalidAuthenticateResponse,
                         "Could not parse decrypted login response plist!");
    }

    auto adsidNode = plist_dict_get_item(decryptedPlist, "adsid");
    auto idmsTokenNode = plist_dict_get_item(decryptedPlist, "GsIdmsToken");

    if (adsidNode == nullptr || idmsTokenNode == nullptr) {
      odslog("ERROR: adsid and /or idmsToken is nil.");
      plist_free(plist);
      throw Exception(ADSError::InvalidAuthenticateResponse,
                         "adsid and /or idmsToken is nil.");
    }

    char *adsid = nullptr;
    plist_get_string_val(adsidNode, &adsid);

    char *idmsToken = nullptr;
    plist_get_string_val(idmsTokenNode, &idmsToken);

    auto statusDictionary = plist_dict_get_item(plist, "Status");
    if (statusDictionary == nullptr) {
      plist_free(plist);
      throw Exception(ADSError::InvalidAuthenticateResponse,
                         "Status is nil.");
    }

    bool isTwoFactorEnabled = false;

    auto authTypeNode = plist_dict_get_item(statusDictionary, "au");
    auto skNode = plist_dict_get_item(decryptedPlist, "sk");
    auto cNode = plist_dict_get_item(decryptedPlist, "c");

    if (authTypeNode != nullptr) {
      char *authType = nullptr;
      plist_get_string_val(authTypeNode, &authType);

      std::string str_auth_type = authType;

      if (str_auth_type == "trustedDeviceSecondaryAuth" ||
          (str_auth_type == "secondaryAuth" /*&& skNode == NULL && cNode == NULL*/)) {
        isTwoFactorEnabled = true;
        // add two factor result
        plist_dict_set_item(decryptedPlist, "twoFactorEnabled",
                            plist_new_bool(true));

        auto task = request_two_factor_auth(adsid, idmsToken);

        auto result = plist_copy(decryptedPlist);
        plist_free(plist);
        free(adsid);
        free(idmsToken);

        return task
            .then([result](bytes data) -> plist_t {
              plist_dict_set_item(result, "twoFactorRequested",
                                  plist_new_bool(true));
              return result;
            })
            .get();
      }
    }

    if (!isTwoFactorEnabled) {
      if (skNode == nullptr || cNode == nullptr) {
        odslog("ERROR: No sk and /or c data.");
        plist_free(plist);
        throw Exception(ADSError::InvalidAuthenticateResponse,
                           "No sk and /or c data.");
      }
    } else {
      // TODO: fetch auth tokens
    }

    // TODO: result
    auto result = plist_copy(decryptedPlist);
    plist_free(plist);
    return result; // include sk, c, twoFactorEnabled, adsid, GsIdmsToken
  };
  return send_auth(parameters).then(complete).then(auth_handler);
}
} // namespace iris