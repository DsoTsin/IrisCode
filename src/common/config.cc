#include "config.hpp"
//#include "rapidjson/document.h"
//#include "rapidjson/prettywriter.h"
#include <fstream>

#if _WIN32
#include <Windows.h>
#endif

namespace iris {
// using namespace rapidjson;
config_value& config_value::operator=(string const& new_value) {
  if (new_value != value) {
    value = new_value;
    provider->notifyDirty(this);
  }
  return *this;
}

config_value& config_value::operator=(config_value const& new_value) {
  if (new_value.value != value) {
    value = new_value.value;
    provider->notifyDirty(this);
  }
  return *this;
}

class registry_provider : public config_provider {
public:
  ~registry_provider() override;
  bool write(string const& key, string const& value) override;
  bool read(string const& key, string& value) const override;
  bool has_key(string const& key) const override;
  string_list keys() const override;
  bool remove_key(string const& key);
  void notifyDirty(config_value* cv) override;
  friend class ibconfig;

private:
  explicit registry_provider(string const& reg_path);

#if _WIN32
  HKEY key_;
  HKEY root_;
#endif
};

ibconfig::ibconfig(string const& path) : provider_(nullptr) {
  if (!strncmp(path.c_str(), "reg:", 4)) {
    provider_ = new registry_provider(path);
  }
}
ibconfig::~ibconfig() {}

config_value& ibconfig::operator[](string const& key) {
  auto& value = cached_kvs_[key];
  value.access_key = key;
  value.provider = provider_;
  provider_->read(key, value.value);
  return value;
}

static const string null_str = "";

string ibconfig::operator[](string const& key) const {
  auto iter = cached_kvs_.find(key);
  if (iter != cached_kvs_.end()) {
    return iter->second.value;
  } else {
    string out_value;
    if (provider_) {
      provider_->read(key, out_value);
      return out_value;
    }
  }
  return null_str;
}

vector<string> ibconfig::keys() const {
  if (provider_) {
    return provider_->keys();
  }
  return vector<string>();
}

ibconfig& ibconfig::remove(string const& key) {
  if (provider_) {
    provider_->remove_key(key);
  }
  return *this;
}

bool ibconfig::has_key(string const& key) const {
  if (provider_) {
    return provider_->has_key(key);
  }
  return false;
}

registry_provider::~registry_provider() {
#if _WIN32
  if (key_) {
    RegCloseKey(key_);
    key_ = 0;
  }
#endif
}
bool registry_provider::write(string const& key, string const& value) {
#if _WIN32
  auto ret
      = RegSetValueExA((HKEY)key_, key.c_str(), 0, REG_SZ, (BYTE*)value.c_str(), value.length());
  return ret == ERROR_SUCCESS;
#else
  return false;
#endif
}
bool registry_provider::read(string const& key, string& value) const {
#if _WIN32
  DWORD length = 0;
  char buffer[1024] = {0};
  LSTATUS ret = RegQueryValueExA((HKEY)key_, key.c_str(), NULL, NULL, NULL, &length);
  if (ret != ERROR_SUCCESS)
    return false;
  ret = RegQueryValueExA((HKEY)key_, key.c_str(), NULL, NULL, (LPBYTE)buffer, &length);
  value = buffer;
  return ret == ERROR_SUCCESS;
#else
  return false;
#endif
}
bool registry_provider::has_key(string const& key) const {
  return false;
}
string_list registry_provider::keys() const {
  return string_list();
}
bool registry_provider::remove_key(string const& key) {
#if _WIN32
  auto ret = ::RegDeleteKeyA((HKEY)key_, key.c_str());
  return ret == ERROR_SUCCESS;
#else
  return false;
#endif
}
void registry_provider::notifyDirty(config_value* cv) {
  write(cv->access_key, cv->value);
}
/*
            enum flags {
                k_query_value = 1,
                k_set_value = 2,
                k_create_sub_key = 4,
                k_enum_sub_keys = 8
            };
*/
registry_provider::registry_provider(string const& reg_path) {
#if _WIN32
  auto sep = reg_path.find_first_of('\\', 5);
  string root = reg_path.substr(5, sep - 5);
  if (root == "HKEY_CURRENT_USER") {
    root_ = HKEY_CURRENT_USER;
  } else if (root == "HKEY_CLASSES_ROOT") {
    root_ = HKEY_CLASSES_ROOT;
  } else if (root == "HKEY_LOCAL_MACHINE") {
    root_ = HKEY_LOCAL_MACHINE;
  } else {
    root_ = nullptr;
  }
  string path = reg_path.substr(sep + 1, reg_path.length() - sep);
  DWORD ret = 0;
  RegCreateKeyExA((HKEY)root_, path.c_str(), NULL, 0, 0, 0, NULL, (PHKEY)&key_, &ret);
  bool isvalid = (ret == REG_CREATED_NEW_KEY || ret == REG_OPENED_EXISTING_KEY);
  if (isvalid && key_) {
    RegCloseKey(key_);
  }
  LSTATUS sret = RegOpenKeyExA((HKEY)root_, path.c_str(), 0, 2 | 8 | 1, (PHKEY)&key_);
  isvalid = sret == ERROR_SUCCESS;
#endif
}
} // namespace iris
