#include "apple_developer_service_api.h"

#include <iostream>

using namespace std;

int main(int argc, const char *argv[]) {
  string appleid;
  if (auto id = getenv("APPLE_ID")) {
    appleid = id;
  }
  string password;
  if (auto pwd = getenv("PSWD_LSAPP")) {
    password = pwd;
  }
  if (appleid.empty()) {
    cout << "AppleID:\n";
    cin >> appleid;
  }
  if (password.empty()) {
    cout << "Password:\n";
    cin >> password;
  }
  ads_error err = nullptr;
  int need_2f = 0;
  auto session = ads_login2(
      appleid.c_str(), password.c_str(),
      [](void *) -> char * {
        static string code;
        cout << "2F code:\n";
        cin >> code;
        return (char *)code.c_str();
      },
      NULL, &err);
  if (err) {
    auto e = ads_err_get_msg(err);
    auto c = ads_err_get_code(err);
    cout << "Code: " << c << ", " << e << endl;
  }

  if (session) {
    auto teams = ads_fetch_teams(session, &err);
    if (err) {
      auto e = ads_err_get_msg(err);
      cout << e << endl;
    }
    int nt = ad_teams_size(teams);
    if (nt > 0) {
      auto tid = ad_teams_get_id(teams, 0);

      auto cert = ads_add_certificates(session, tid, "ZhouiPAD", &err);
      if (err) {
        auto e = ads_err_get_msg(err);
        cout << "Add certs error: " << e << endl;
      }

#if 0
      ads_revoke_certificate(session, tid, "A7VKX43M44", &err);
      if (err) {
        auto e = ads_err_get_msg(err);
        cout << "revoke certs error: " << e << endl;
      }
#endif

      // Fetch certs not worked
      auto certs = ads_fetch_certificates(session, tid, &err);
      if (err) {
        auto e = ads_err_get_msg(err);
        cout << "Fetch certs error: " << e << endl;
      }
      if (certs) {
        int ncerts = ad_certs_size(certs);
        for (int i = 0; i < ncerts; i++) {
          auto id = ad_certs_get_id(certs, i);
          auto name = ad_certs_get_name(certs, i);
          cout << "\tCert => name=" << name << " id=" << id << endl;
        }
        ad_certs_free(certs);
      }
      auto apps = ads_fetch_apps(session, tid, &err);
      if (err) {
        auto e = ads_err_get_msg(err);
        cout << "Fetch appIds error: " << e << endl;
      }

#if 0
      if (apps) {
        auto num_apps = ad_apps_size(apps);
        if (num_apps > 0) {
          cout << "List Apps:\n";
        }
        for (int i = 0; i < num_apps; i++) {
          auto id = ad_apps_get_id(apps, i);
          auto name = ad_apps_get_name(apps, i);
          auto bid = ad_apps_get_identifier(apps, i);
          cout << "\tID: " << id << " Name: " << name << " BundleID: " << bid
               << endl;

          auto prof = ads_fetch_provision_profile(session, tid, id,
                                                  ads_device_type_iphone, &err);

          if (prof) {
            auto id = ad_provision_profile_get_id(prof);
            auto type = ad_provision_profile_get_type(prof);
            auto name = ad_provision_profile_get_name(prof);
            auto bid = ad_provision_profile_get_bundle_id(prof);
            cout << "\t\tProvision Profile -- Id: " << id << " Type: " << type
                 << " Name: " << name << " BundleID: " << bid << endl;
            ad_provision_profile_free(prof);
            prof = nullptr;
          }
        }
        ad_apps_free(apps);
      }
#endif

      auto app_groups = ads_fetch_app_groups(session, tid, &err);
      if (err) {
        auto e = ads_err_get_msg(err);
        cout << "Fetch appGroups error: " << e << endl;
      }

      auto devs = ads_fetch_devices(session, tid, &err);
      if (err) {
        auto e = ads_err_get_msg(err);
        cout << "Fetch devices error: " << e << endl;
      }
      int nd = ad_devices_size(devs);
      if (nd > 0) {
        cout << "List Devices:\n";
        for (int i = 0; i < nd; i++) {
          auto name = ad_devices_get_name(devs, i);
          extern const char *utf8_ansi(const char *sAnsi);
          cout << "\t" << utf8_ansi(name) << endl;
        }
        ad_devices_free(devs);
      }

      // ads_register_device(session, tid, "xxx", "xxxx-ssdd", iphone, &err);
    }
    ads_free(session);
  }
  return 0;
}

#include <Windows.h>

const char *utf8_ansi(const char *ustr) {
  static thread_local wchar_t buff[4096] = {0};
  static thread_local char cbuff[4096] = {0};
  int sLen = MultiByteToWideChar(CP_UTF8, NULL, ustr, -1, NULL, 0);
  if (sLen < 4096) {
    MultiByteToWideChar(CP_UTF8, NULL, ustr, -1, buff, sLen);
  } else {
    // TODO:
  }
  sLen = WideCharToMultiByte(CP_ACP, 0, buff, -1, NULL, NULL, NULL, NULL);
  WideCharToMultiByte(CP_ACP, 0, buff, -1, cbuff, sLen, NULL, NULL);
  return cbuff;
}