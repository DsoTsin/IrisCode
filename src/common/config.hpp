#pragma once
#include "stl.hpp"

namespace iris {
//class global_config {
//public:
//  // address of coordinator, ip or host
//  string coordinator;
//  // tcp port of service, coordinator port is (build_service_port-1)
//  int build_service_port;
//
//  bool operator==(global_config const& rhs) const;
//  static global_config default();
//};

struct config_provider;

struct config_value {
  string value;
  config_value& operator=(string const& new_value);
  config_value& operator=(config_value const& new_value);
  operator string() const { return value; }

private:
  string access_key;
  config_provider* provider;
  friend class ibconfig;
  friend class registry_provider;
};

struct config_provider {
  virtual ~config_provider() {}
  virtual bool write(string const& key, string const& value) = 0;
  virtual bool read(string const& key, string& value) const = 0;
  virtual bool has_key(string const& key) const = 0;
  virtual bool remove_key(string const& key) = 0;
  virtual string_list keys() const = 0;
  virtual void notifyDirty(config_value*) {}
};

class ibconfig {
public:
  explicit ibconfig(string const& path);
  virtual ~ibconfig();

  config_value& operator[](string const& key);
  string operator[](string const& key) const;
  vector<string> keys() const;

  ibconfig& remove(string const& key);
  bool has_key(string const& key) const;

private:
  std::map<string, config_value> cached_kvs_;
  config_provider* provider_;
};
} // namespace iris
