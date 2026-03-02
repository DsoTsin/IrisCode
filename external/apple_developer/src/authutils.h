#pragma once

#include "common_priv.h"

struct ccdigest_info;
struct ccdigest_ctx;
struct ccsrp_ctx;

#define odslog(msg)                                                            \
  {                                                                            \
    std::stringstream ss;                                                      \
    ss << msg << std::endl;                                                    \
    ::iris::minilog(ss.str().c_str());                                      \
  }

namespace iris {
template <typename T> class vector_view {
  const T *ptr_;
  std::size_t len_;

public:
  vector_view(const T *ptr, std::size_t len) noexcept : ptr_{ptr}, len_{len} {}
  vector_view(const std::vector<T> &arr) noexcept
      : ptr_{arr.data()}, len_{arr.size()} {}

  T &operator[](int i) noexcept { return ptr_[i]; }
  T const &operator[](int i) const noexcept { return ptr_[i]; }
  auto size() const noexcept { return len_; }

  auto begin() noexcept { return ptr_; }
  auto end() noexcept { return ptr_ + len_; }

  const T *data() const { return ptr_; }

  std::vector<T> clone() const { return std::vector<T>(ptr_, ptr_ + len_); }
};
using bytes = std::vector<uint8_t>;
using bytes_view = vector_view<uint8_t>;
using opt_bytes = std::optional<bytes>;

std::wstring to_utf16(std::string const &str);
std::string to_utf8(std::wstring const &wstr);
std::string make_uuid();
bool gzip_decompress(const bytes_view &compressedBytes, std::string &uncompressedBytes);
void minilog(const char *log);

opt_bytes ALTPBKDF2SRP(const ccdigest_info *di_info, bool isS2k,
                       const std::string &password, const bytes_view &salt,
                       int iterations);
void ALTDigestUpdateString(const ccdigest_info *di_info, ccdigest_ctx *di_ctx,
                           const std::string &string);

bytes ALTCreateSessionKey(ccsrp_ctx *srp_ctx, const char *key_name);
opt_bytes ALTDecryptDataCBC(ccsrp_ctx *srp_ctx, const bytes_view &spd);
// TODO
opt_bytes ALTDecryptDataGCM(const bytes_view &,
                            const bytes_view &encryptedData);
bytes ALTCreateAppTokensChecksum(const bytes_view &sk, const std::string &adsid,
                                 const std::vector<std::string> &apps);

class ALTDigest {
public:
  ALTDigest();

  ~ALTDigest();

  void update_string(const std::string &str);
  void update_data(const vector_view<uint8_t> &data);

  std::vector<uint8_t> start_client_auth();

  void client_process_challege(const std::string &account,
                               const std::string &password,
                               const bytes_view &salt, const bytes_view &pbk,
                               int iterations, bool isS2k, bytes &outkey);

  bool verify_session(const bytes_view &m2);

  void final_digest(bytes &digest);

  opt_bytes decrypt_cbc(const vector_view<uint8_t> &in);

private:
  opt_bytes PBKDF2SRP(bool isS2K, const std::string &password, bytes &salt,
                      int iterations);

  const ccdigest_info *di_info_;
  ccdigest_ctx *di_ctx_;
  const ccdigest_info *srp_di_;
  ccsrp_ctx *srp_ctx_;
};
} // namespace iris