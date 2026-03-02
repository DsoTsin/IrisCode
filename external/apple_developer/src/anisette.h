#pragma once

#include "common_priv.h"

namespace iris {
extern std::string base64(const unsigned char *input, int length);
class Anisette {
public:
  struct TimeValue {
    long tv_sec;  /* seconds */
    long tv_usec; /* and microseconds */
  };

public:
  static Anisette &get();

  void provision();

  std::map<std::string, plist_t> make_dict();
  std::map<std::string, std::string> make_string_dict(bool with_srl = true);

  const std::string &locale() const { return locale_; }
  const std::string &deviceDescription() const { return deviceDescription_; }

private:
  Anisette();
  ~Anisette();

  bool try_init();

  bool load_icloud();
  bool load_dependencies();
  bool reprovision();

  bool inited_;
  int reprov_count_;

  std::mutex prov_lock;

  std::string machineID_;
  std::string oneTimePassword_;
  std::string localUserID_;
  unsigned long long routingInfo_;
  std::string deviceUniqueIdentifier_;
  std::string deviceSerialNumber_;
  std::string deviceDescription_;
  TimeValue date_;
  std::string locale_;
  std::string timeZone_;
};
} // namespace iris