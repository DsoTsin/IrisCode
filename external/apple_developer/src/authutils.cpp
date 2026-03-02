#include "authutils.h"
#include "anisette.h"
#include <Windows.h>
#include <cpprest/http_client.h>
#include <iomanip>
#include <sstream>
#include <zlib.h>
extern "C" {
#include <corecrypto/ccaes.h>
#include <corecrypto/ccdigest.h>
#include <corecrypto/ccdrbg.h>
#include <corecrypto/cchmac.h>
#include <corecrypto/ccpad.h>
#include <corecrypto/ccpbkdf2.h>
#include <corecrypto/ccsha2.h>
#include <corecrypto/ccsrp.h>
#include <corecrypto/ccsrp_gp.h>
}
namespace iris {
void minilog(const char *log) {
#ifdef _DEBUG
  ::OutputDebugStringA(log);
#endif
}

bytes DataFromBytes(const char *inbytes, size_t count) {
  bytes data;
  data.reserve(count);
  for (int i = 0; i < count; i++) {
    data.push_back(inbytes[i]);
  }
  return data;
}

static const char ALTHexCharacters[] = "0123456789abcdef";

std::optional<std::vector<unsigned char>>
ALTPBKDF2SRP(const struct ccdigest_info *di_info, bool isS2k,
             const std::string &password, const bytes_view &salt,
             int iterations) {
  const struct ccdigest_info *password_di_info = ccsha256_di();
  char *digest_raw = (char *)alloca(password_di_info->output_size);
  ccdigest(password_di_info, password.length(), password.c_str(), digest_raw);

  size_t final_digest_len = password_di_info->output_size * (isS2k ? 1 : 2);
  char *digest = (char *)alloca(final_digest_len);

  if (isS2k) {
    memcpy(digest, digest_raw, final_digest_len);
  } else {
    for (size_t i = 0; i < password_di_info->output_size; i++) {
      char byte = digest_raw[i];
      digest[i * 2 + 0] = ALTHexCharacters[(byte >> 4) & 0x0F];
      digest[i * 2 + 1] = ALTHexCharacters[(byte >> 0) & 0x0F];
    }
  }

  char *outputBytes = (char *)alloca(di_info->output_size);
  if (ccpbkdf2_hmac(di_info, final_digest_len, digest, salt.size(), salt.data(),
                    iterations, di_info->output_size, outputBytes) != 0) {
    return std::nullopt;
  }

  auto data = bytes(outputBytes, outputBytes + di_info->output_size);
  return std::make_optional(data);
}

void ALTDigestUpdateString(const struct ccdigest_info *di_info,
                           struct ccdigest_ctx *di_ctx,
                           const std::string &string) {
  ccdigest_update(di_info, di_ctx, string.length(), string.c_str());
}

void ALTDigestUpdateData(const struct ccdigest_info *di_info,
                         struct ccdigest_ctx *di_ctx, bytes &data) {
  uint32_t data_len = (uint32_t)data.size(); // 4 bytes for length
  ccdigest_update(di_info, di_ctx, sizeof(data_len), &data_len);
  ccdigest_update(di_info, di_ctx, data_len, data.data());
}

bytes ALTCreateSessionKey(ccsrp_ctx_t srp_ctx, const char *key_name) {
  size_t key_len;
  const void *session_key = ccsrp_get_session_key(srp_ctx, &key_len);

  const struct ccdigest_info *di_info = ccsha256_di();

  size_t length = strlen(key_name);

  size_t hmac_len = di_info->output_size;
  unsigned char *hmac_bytes = (unsigned char *)alloca(hmac_len);
  cchmac(di_info, key_len, session_key, length, key_name, hmac_bytes);

  auto data = bytes(hmac_bytes, hmac_bytes + hmac_len);
  return data;
}

opt_bytes ALTDecryptDataCBC(ccsrp_ctx_t srp_ctx, const bytes_view &spd) {
  auto extraDataKey = ALTCreateSessionKey(srp_ctx, "extra data key:");
  auto extraDataIV = ALTCreateSessionKey(srp_ctx, "extra data iv:");

  bytes decryptedBytes;
  decryptedBytes.resize(spd.size());
  const struct ccmode_cbc *decrypt_mode = ccaes_cbc_decrypt_mode();

  cccbc_iv *iv = (cccbc_iv *)alloca(decrypt_mode->block_size);
  if (iv) {
    if (extraDataIV.data()) {
      memcpy(iv, extraDataIV.data(), decrypt_mode->block_size);
    } else {
      memset(iv, 0, decrypt_mode->block_size);
    }
  } else {
    // TODO
  }
  cccbc_ctx *ctx_buf = (cccbc_ctx *)alloca(decrypt_mode->size);
  decrypt_mode->init(decrypt_mode, ctx_buf, extraDataKey.size(),
                     extraDataKey.data());

  size_t length = ccpad_pkcs7_decrypt(decrypt_mode, ctx_buf, iv, spd.size(),
                                      spd.data(), decryptedBytes.data());

  if (length > spd.size()) {
    return std::nullopt;
  }

  auto decryptedData =
      bytes(decryptedBytes.data(), decryptedBytes.data() + length);
  return decryptedData;
}

opt_bytes ALTDecryptDataGCM(const bytes_view &sk,
                            const bytes_view &encryptedData) {
  const struct ccmode_gcm *decrypt_mode = ccaes_gcm_decrypt_mode();

  ccgcm_ctx *gcm_ctx = (ccgcm_ctx *)alloca(decrypt_mode->size);
  decrypt_mode->init(decrypt_mode, gcm_ctx, sk.size(), sk.data());

  if (encryptedData.size() < 35) {
    odslog("ERROR: Encrypted token too short.");
    return std::nullopt;
  }

  if (cc_cmp_safe(3, encryptedData.data(), "XYZ")) {
    odslog("ERROR: Encrypted token wrong version!");
    return std::nullopt;
  }

  decrypt_mode->set_iv(gcm_ctx, 16, encryptedData.data() + 3);
  decrypt_mode->gmac(gcm_ctx, 3, encryptedData.data());

  size_t decrypted_len = encryptedData.size() - 35;
  bytes decryptedBytes(decrypted_len);
  // char *decryptedBytes = (char *)malloc(decrypted_len);
  decrypt_mode->gcm(gcm_ctx, decrypted_len, encryptedData.data() + 16 + 3,
                    decryptedBytes.data());

  char tag[16];
  decrypt_mode->finalize(gcm_ctx, 16, tag);

  if (cc_cmp_safe(16, encryptedData.data() + decrypted_len + 19, tag)) {
    odslog("ERROR: Invalid tag version.");
    return std::nullopt;
  }
  // auto decryptedData =
  //    DataFromBytes((const char *)decryptedBytes, decrypted_len);
  return decryptedBytes;
}

bytes ALTCreateAppTokensChecksum(const bytes_view &sk, const std::string &adsid,
                                 const std::vector<std::string> &apps) {
  const struct ccdigest_info *di_info = ccsha256_di();
  size_t hmac_size = cchmac_di_size(di_info);
  struct cchmac_ctx *hmac_ctx = (struct cchmac_ctx *)alloca(hmac_size);
  cchmac_init(di_info, hmac_ctx, sk.size(), sk.data());

  const char *key = "apptokens";
  cchmac_update(di_info, hmac_ctx, strlen(key), key);

  cchmac_update(di_info, hmac_ctx, adsid.length(), adsid.c_str());

  for (auto app : apps) {
    cchmac_update(di_info, hmac_ctx, app.length(), app.c_str());
  }

  char *checksumBytes = (char *)alloca(di_info->output_size);
  cchmac_final(di_info, hmac_ctx, (unsigned char *)checksumBytes);

  auto checksum = bytes(checksumBytes, checksumBytes + di_info->output_size);
  return checksum;
}

std::wstring to_utf16(std::string const &str) {
  if (str.empty())
    return std::wstring();
  size_t charsNeeded =
      ::MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0);
  if (charsNeeded == 0)
    return std::wstring();
  LPWSTR buffer = (LPWSTR)malloc((charsNeeded + 1) * 2);
  int charsConverted = ::MultiByteToWideChar(
      CP_UTF8, 0, str.data(), (int)str.size(), buffer, charsNeeded);
  buffer[charsNeeded] = 0;
  if (charsConverted == 0)
    return std::wstring();
  auto res = std::wstring(buffer, charsConverted);
  free(buffer);
  return res;
}

std::string to_utf8(std::wstring const &wstr) {
  if (wstr.empty())
    return std::string();
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(),
                                        NULL, 0, NULL, NULL);
  std::string strTo(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0],
                      size_needed, NULL, NULL);
  return strTo;
}

std::string make_uuid() {
  GUID guid;
  CoCreateGuid(&guid);

  std::ostringstream os;
  os << std::hex << std::setw(8) << std::setfill('0') << guid.Data1;
  os << '-';
  os << std::hex << std::setw(4) << std::setfill('0') << guid.Data2;
  os << '-';
  os << std::hex << std::setw(4) << std::setfill('0') << guid.Data3;
  os << '-';
  os << std::hex << std::setw(2) << std::setfill('0')
     << static_cast<short>(guid.Data4[0]);
  os << std::hex << std::setw(2) << std::setfill('0')
     << static_cast<short>(guid.Data4[1]);
  os << '-';
  os << std::hex << std::setw(2) << std::setfill('0')
     << static_cast<short>(guid.Data4[2]);
  os << std::hex << std::setw(2) << std::setfill('0')
     << static_cast<short>(guid.Data4[3]);
  os << std::hex << std::setw(2) << std::setfill('0')
     << static_cast<short>(guid.Data4[4]);
  os << std::hex << std::setw(2) << std::setfill('0')
     << static_cast<short>(guid.Data4[5]);
  os << std::hex << std::setw(2) << std::setfill('0')
     << static_cast<short>(guid.Data4[6]);
  os << std::hex << std::setw(2) << std::setfill('0')
     << static_cast<short>(guid.Data4[7]);

  return os.str();
}

bool gzip_decompress(const bytes_view &compressedBytes,
                     std::string &uncompressedBytes) {
  if (compressedBytes.size() == 0) {
    uncompressedBytes =
        std::string(compressedBytes.data(),
                    compressedBytes.data() + compressedBytes.size());
    return true;
  }

  uncompressedBytes.clear();

  unsigned full_length = compressedBytes.size();
  unsigned half_length = compressedBytes.size() / 2;

  unsigned uncompLength = full_length;
  char *uncomp = (char *)malloc(uncompLength);

  z_stream strm;
  strm.next_in = (Bytef *)compressedBytes.data();
  strm.avail_in = compressedBytes.size();
  strm.total_out = 0;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;

  bool done = false;

  if (inflateInit2(&strm, (16 + MAX_WBITS)) != Z_OK) {
    free(uncomp);
    return false;
  }

  while (!done) {
    // If our output buffer is too small
    if (strm.total_out >= uncompLength) {
      // Increase size of output buffer
      char *uncomp2 = (char *)malloc(uncompLength + half_length);
      memcpy(uncomp2, uncomp, uncompLength);
      uncompLength += half_length;
      free(uncomp);
      uncomp = uncomp2;
    }

    strm.next_out = (Bytef *)(uncomp + strm.total_out);
    strm.avail_out = uncompLength - strm.total_out;

    // Inflate another chunk.
    int err = inflate(&strm, Z_SYNC_FLUSH);
    if (err == Z_STREAM_END)
      done = true;
    else if (err != Z_OK) {
      break;
    }
  }

  if (inflateEnd(&strm) != Z_OK) {
    free(uncomp);
    return false;
  }

  // TODO: optimize
  for (size_t i = 0; i < strm.total_out; ++i) {
    uncompressedBytes.push_back(uncomp[i]);
  }
  free(uncomp);
  return true;
}

ALTDigest::ALTDigest() {
  ccsrp_const_gp_t gp = ccsrp_gp_rfc5054_2048();
  di_info_ = ccsha256_di();
  di_ctx_ = (struct ccdigest_ctx *)malloc(ccdigest_di_size(di_info_));
  ccdigest_init(di_info_, di_ctx_);
  srp_di_ = ccsha256_di();
  srp_ctx_ = (ccsrp_ctx_t)malloc(ccsrp_sizeof_srp(di_info_, gp));
  ccsrp_ctx_init(srp_ctx_, srp_di_, gp);
  HDR(srp_ctx_)->blinding_rng = ccrng(NULL);
  HDR(srp_ctx_)->flags.noUsernameInX = true;
}

ALTDigest::~ALTDigest() {}

void ALTDigest::update_string(const std::string &str) {
  ccdigest_update(di_info_, di_ctx_, str.length(), str.c_str());
}

void ALTDigest::update_data(const vector_view<uint8_t> &data) {
  uint32_t data_len = (uint32_t)data.size(); // 4 bytes for length
  ccdigest_update(di_info_, di_ctx_, sizeof(data_len), &data_len);
  ccdigest_update(di_info_, di_ctx_, data_len, &data[0]);
}

std::vector<uint8_t> ALTDigest::start_client_auth() {
  size_t A_size = ccsrp_exchange_size(srp_ctx_);
  std::vector<uint8_t> A_bytes;
  A_bytes.resize(A_size);
  ccsrp_client_start_authentication(srp_ctx_, ccrng(NULL), A_bytes.data());
  return A_bytes;
}

void ALTDigest::client_process_challege(const std::string &account,
                                        const std::string &password,
                                        const bytes_view &salt,
                                        const bytes_view &pbk, int iterations,
                                        bool isS2k, bytes &outkey) {
  bytes templ_salt = salt.clone();
  auto passwordKey =
      ALTPBKDF2SRP(di_info_, isS2k, password, templ_salt, iterations);
  if (passwordKey == std::nullopt) {
    // throw APIError(APIErrorCode::AuthenticationHandshakeFailed);
  }
  size_t M_size = ccsrp_get_session_key_length(srp_ctx_);
  outkey.resize(M_size);
  int result = ccsrp_client_process_challenge(
      srp_ctx_, account.c_str(), passwordKey->size(), passwordKey->data(),
      salt.size(), templ_salt.data(), &pbk[0], outkey.data());
  if (result != 0) {
    // throw APIError(APIErrorCode::AuthenticationHandshakeFailed);
  }
}

bool ALTDigest::verify_session(const bytes_view &m2) {
  return ccsrp_client_verify_session(srp_ctx_, m2.data()) != 0;
}

void ALTDigest::final_digest(bytes &digest) {
  digest.resize(di_info_->output_size);
  di_info_->final(di_info_, di_ctx_, digest.data());
}

opt_bytes ALTDigest::decrypt_cbc(const vector_view<uint8_t> &in) {
  return ALTDecryptDataCBC(srp_ctx_, in);
}

opt_bytes ALTDigest::PBKDF2SRP(bool isS2k, const std::string &password,
                               bytes &salt, int iterations) {
  const struct ccdigest_info *password_di_info = ccsha256_di();
  bytes digest_raw;
  digest_raw.resize(password_di_info->output_size);
  ccdigest(password_di_info, password.length(), password.c_str(),
           digest_raw.data());

  size_t final_digest_len = password_di_info->output_size * (isS2k ? 1 : 2);
  char *digest = (char *)alloca(final_digest_len);

  if (isS2k) {
    memcpy(digest, digest_raw.data(), final_digest_len);
  } else {
    for (size_t i = 0; i < password_di_info->output_size; i++) {
      char byte = digest_raw[i];
      digest[i * 2 + 0] = ALTHexCharacters[(byte >> 4) & 0x0F];
      digest[i * 2 + 1] = ALTHexCharacters[(byte >> 0) & 0x0F];
    }
  }

  bytes outputBytes;
  outputBytes.resize(di_info_->output_size);
  if (ccpbkdf2_hmac(di_info_, final_digest_len, digest, salt.size(),
                    salt.data(), iterations, di_info_->output_size,
                    outputBytes.data()) != 0) {
    return std::nullopt;
  }
  return std::make_optional(outputBytes);
}
} // namespace iris