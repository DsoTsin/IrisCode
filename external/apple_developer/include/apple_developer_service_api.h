#ifndef __apple_developer_service_api_h__
#define __apple_developer_service_api_h__
#pragma once

#ifdef _MSC_VER
#ifdef BUILD_LIB
#define ADSAPI __declspec(dllexport)
#else
#define ADSAPI __declspec(dllimport)
#endif
#else
#define ADSAPI __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _ads_device_type {
  ads_device_type_iphone = 1,
  ads_device_type_ipad = 2,
  ads_device_type_tv = 4,
  ads_device_type_all = 7,
  ads_device_type_none = 0
} ads_device_type;
// Apple Developer Service session
typedef struct _ads_session {
  int need_2_factor_auth;
} ads_session;
typedef void *ads_error;
typedef void *ad_team;
typedef void *ad_teams;
typedef void *ad_device;
typedef void *ad_devices;
typedef void *ad_app;
typedef void *ad_apps;
typedef void *ad_app_group;
typedef void *ad_app_groups;
typedef void *ad_cert;
typedef void *ad_certs;
typedef void *ad_provisioning_profile;

// Get error message
ADSAPI const char *ads_err_get_msg(ads_error err);
ADSAPI int ads_err_get_code(ads_error err);
ADSAPI void ads_err_free(ads_error *err);

ADSAPI ads_session* ads_login(const char* apple_id, const char* password, int* need_verify, ads_error* error);
ADSAPI void ads_verify(ads_session* session, const char* code, ads_error* error);


typedef char *(*verify_callback)(void *);
ADSAPI ads_session *ads_login2(const char *apple_id, const char *password,
                               verify_callback callback, void *usr_data,
                               ads_error *error);
ADSAPI void ads_free(ads_session *session);

ADSAPI int ad_teams_size(ad_teams teams);
ADSAPI void ad_teams_free(ad_teams teams);
ADSAPI const char *ad_teams_get_id(ad_teams teams, int index);
ADSAPI const char *ad_teams_get_name(ad_teams teams, int index);
typedef enum ad_developer_type {
    ad_developer_type_unknown = 0,
    ad_developer_type_free = 1,
    ad_developer_type_individual = 2,
    ad_developer_type_organization = 3
} ad_developer_type;
ADSAPI ad_developer_type ad_teams_get_type(ad_teams teams, int index);

ADSAPI ad_teams ads_fetch_teams(ads_session *session, ads_error *error);

ADSAPI int ad_devices_size(ad_devices devices);
ADSAPI void ad_devices_free(ad_devices devices);
ADSAPI const char *ad_devices_get_id(ad_devices devs, int index);
ADSAPI const char* ad_devices_get_name(ad_devices devs, int index);
ADSAPI const char* ad_devices_get_machine_id(ad_devices devs, int index);
ADSAPI const char* ad_devices_get_model(ad_devices devs, int index);

ADSAPI ad_devices ads_fetch_devices(ads_session *session, const char *team_id,
                                    ads_error *error);
ADSAPI ad_device ads_register_device(ads_session *session, const char *team_id,
                                     const char *mach_name, const char *mach_id,
                                     ads_device_type dtype, ads_error *error);

ADSAPI void ad_certs_free(ad_certs certs);
ADSAPI int ad_certs_size(ad_certs certs);
ADSAPI const char *ad_certs_get_id(ad_certs certs, int index);
ADSAPI const char *ad_certs_get_serial_no(ad_certs certs, int index);
ADSAPI const char* ad_certs_get_name(ad_certs certs, int index);
ADSAPI const char* ad_certs_get_machine_name(ad_certs certs, int index);
ADSAPI const char* ad_certs_get_machine_id(ad_certs certs, int index);
ADSAPI const char* ad_certs_get_requester_email(ad_certs certs, int index);
ADSAPI const char* ad_certs_get_expiration_date(ad_certs certs, int index);
ADSAPI const char* ad_certs_get_data(ad_certs certs, int index);
ADSAPI const char* ad_certs_get_priv_key(ad_certs certs, int index);
ADSAPI size_t ad_certs_get_data_len(ad_certs certs, int index);
ADSAPI size_t ad_certs_get_priv_key_len(ad_certs certs, int index);

ADSAPI ad_certs ads_fetch_certificates(ads_session *session,
                                       const char *team_id, ads_error *error);
ADSAPI ad_cert ads_add_certificates(ads_session *session, const char *team_id,
                                    const char *mach_name, ads_error *error);
ADSAPI int ads_revoke_certificate(ads_session *session, const char *team_id,
                                  const char *cert_id, ads_error *error);

ADSAPI int ad_apps_size(ad_apps apps);
ADSAPI void ad_apps_free(ad_apps apps);
ADSAPI const char *ad_apps_get_name(ad_apps apps, int index);
ADSAPI const char *ad_apps_get_id(ad_apps apps, int index);
ADSAPI const char *ad_apps_get_bundle_id(ad_apps apps, int index);
ADSAPI ad_apps ads_fetch_apps(ads_session *session, const char *team_id,
                              ads_error *error);

ADSAPI ad_app_groups ads_fetch_app_groups(ads_session *session,
                                          const char *team_id,
                                          ads_error *error);

ADSAPI const char *
ad_provision_profile_get_id(ad_provisioning_profile profile);
ADSAPI const char *
ad_provision_profile_get_name(ad_provisioning_profile profile);
ADSAPI const char *
ad_provision_profile_get_type(ad_provisioning_profile profile);
ADSAPI const char *
ad_provision_profile_get_bundle_id(ad_provisioning_profile profile);
ADSAPI const char *ad_provision_profile_data(ad_provisioning_profile profile);
ADSAPI int ad_provision_profile_size(ad_provisioning_profile profile);
ADSAPI void ad_provision_profile_free(ad_provisioning_profile profile);
ADSAPI ad_provisioning_profile ads_fetch_provision_profile(
    ads_session *session, const char *team_id, const char *app_id,
    ads_device_type dtype, ads_error *error);

#ifdef __cplusplus
}
#ifdef __UNREAL__
#include "CoreMinimal.h"
#include "Misc/MonitoredProcess.h"

namespace AAPL {
struct Device {
    FString ID;
    FString Name;
    FString MachineID;
    FString Model;
};

using DevicePtr = TSharedPtr<Device>;

struct Cert {
    FString ID;
    FString SerialNo;
    FString Name;
    FString MachineName;
    FString MachineID;
    FString ExpirationDate;
    FString RequesterEMail;
    TArray<uint8> Data;
    TArray<uint8> PK;
};

using CertPtr = TSharedPtr<Cert>;

struct App {
    FString ID;
    FString BundleID;
    FString Name;
};

using AppPtr = TSharedPtr<App>;

struct ProvProf {
    FString ID;
    FString Name;
    FString Type;
    FString BundleID;
    TArray<uint8> Data;
};

using ProvProfPtr = TSharedPtr<ProvProf>;

struct Team {
    FString ID;
    FString Name;
    ad_developer_type Type;
    TArray<Device> Devices;
    TArray<Cert> Certs;
    TArray<App> Apps;
};

template <class T>
struct Result {
    T Ok;
    FString Error;
    int32 Code;
};

class Session {
public:
    static FMonitoredProcess& Initialize()
    {
        static FMonitoredProcess Proc(FPaths::EngineDir() / TEXT("Binaries/Win64/ads_daemon.exe"), TEXT(""), true);
        if (!Proc.Update()) {
            Proc.Launch();
        }
        return Proc;
    }

    static void Finalize() 
    { 
        Initialize().Exit();
    }

    Session()
        : m_Session(nullptr)
        , m_Error(nullptr)
        , m_NeedVerify(0)
    {
    }

    ~Session()
    {
        Reset();
    }

    void Login(const FString& Account, const FString& Password, FString* Error, int32* Code)
    {
        m_Session = ads_login(TCHAR_TO_UTF8(*Account), TCHAR_TO_UTF8(*Password), &m_NeedVerify, &m_Error);
        if (m_Error && Error && Code) {
            *Error = UTF8_TO_TCHAR(ads_err_get_msg(m_Error));
            *Code = ads_err_get_code(m_Error);
            ads_err_free(&m_Error);
            m_Error = nullptr;
        }
    }

    void Verify(const FString& Code, FString* Error, int32* ECode)
    {
        if (m_Session && m_NeedVerify) {
            ads_verify(m_Session, TCHAR_TO_UTF8(*Code), &m_Error);
            if (m_Error && Error && ECode) {
                *Error = UTF8_TO_TCHAR(ads_err_get_msg(m_Error));
                *ECode = ads_err_get_code(m_Error);
                ads_err_free(&m_Error);
                m_Error = nullptr;
            }
        }
    }

    void Reset()
    {
        if (m_Session) {
            ads_free(m_Session);
            m_Session = nullptr;
        }

        if (m_Error) {
            ads_err_free(&m_Error);
            m_Error = nullptr;
        }
        m_NeedVerify = 0;
    }

    bool IsValid() const
    {
        return m_Session != nullptr;
    }

    void GetLastError(FString& ErrMsg, int32& Code)
    {
        if (m_Error) {
            ErrMsg = UTF8_TO_TCHAR(ads_err_get_msg(m_Error));
            Code = ads_err_get_code(m_Error);
            ads_err_free(&m_Error);
            m_Error = nullptr;
        }
    }

    bool _2FA() const { return m_NeedVerify == 1; }

    void FetchTeams(Result<TArray<AAPL::Team>>& OutTeams)
    {
        if (m_Session) {
            auto Teams = ads_fetch_teams(m_Session, &m_Error);
            if (m_Error) {
                OutTeams.Error = UTF8_TO_TCHAR(ads_err_get_msg(m_Error));
                OutTeams.Code = ads_err_get_code(m_Error);
                ads_err_free(&m_Error);
                m_Error = nullptr;
            }
            if (Teams) {
                for (int32 i = 0; i < ad_teams_size(Teams); i++) {
                    AAPL::Team team;
                    team.ID = UTF8_TO_TCHAR(ad_teams_get_id(Teams, i));
                    team.Name = UTF8_TO_TCHAR(ad_teams_get_name(Teams, i));
                    team.Type = ad_teams_get_type(Teams, i);
                    OutTeams.Ok.Add(team);
                }
                ad_teams_free(Teams);
            }
        }
    }

    void FetchDevices(const AAPL::Team& Team, Result<TArray<Device>>& OutDevices)
    {
        if (m_Session) {
            const char* tid = TCHAR_TO_UTF8(*Team.ID);
            auto Devices = ads_fetch_devices(m_Session, tid, &m_Error);
            if (m_Error) {
                OutDevices.Error = UTF8_TO_TCHAR(ads_err_get_msg(m_Error));
                OutDevices.Code = ads_err_get_code(m_Error);
                ads_err_free(&m_Error);
                m_Error = nullptr;
            }
            if (Devices) {
                for (int32 i = 0; i < ad_devices_size(Devices); i++) {
                    AAPL::Device dev;
                    dev.ID = UTF8_TO_TCHAR(ad_devices_get_id(Devices, i));
                    dev.Name = UTF8_TO_TCHAR(ad_devices_get_name(Devices, i));
                    dev.MachineID = UTF8_TO_TCHAR(ad_devices_get_machine_id(Devices, i));
                    dev.Model = UTF8_TO_TCHAR(ad_devices_get_model(Devices, i));
                    OutDevices.Ok.Add(dev);
                }
                ad_devices_free(Devices);
            }
        }
    }

    void FetchCerts(const AAPL::Team& Team, Result<TArray<Cert>>& OutCerts)
    {
        if (m_Session) {
            const char* tid = TCHAR_TO_UTF8(*Team.ID);
            auto Certs = ads_fetch_certificates(m_Session, tid, &m_Error);
            if (m_Error) {
                OutCerts.Error = UTF8_TO_TCHAR(ads_err_get_msg(m_Error));
                OutCerts.Code = ads_err_get_code(m_Error);
                ads_err_free(&m_Error);
                m_Error = nullptr;
            }
            if (Certs) {
                for (int32 i = 0; i < ad_certs_size(Certs); i++) {
                    AAPL::Cert cert;
                    cert.ID = UTF8_TO_TCHAR(ad_certs_get_id(Certs, i));
                    cert.Name = UTF8_TO_TCHAR(ad_certs_get_name(Certs, i));
                    cert.SerialNo = UTF8_TO_TCHAR(ad_certs_get_serial_no(Certs, i));
                    cert.MachineID = UTF8_TO_TCHAR(ad_certs_get_machine_id(Certs, i));
                    cert.MachineName = UTF8_TO_TCHAR(ad_certs_get_machine_name(Certs, i));
                    cert.ExpirationDate = UTF8_TO_TCHAR(ad_certs_get_expiration_date(Certs, i));
                    cert.RequesterEMail = UTF8_TO_TCHAR(ad_certs_get_requester_email(Certs, i));
                    cert.Data = TArray<uint8>((const uint8*)ad_certs_get_data(Certs, i),
                                              ad_certs_get_data_len(Certs, i));
                    cert.PK = TArray<uint8>((const uint8*)ad_certs_get_priv_key(Certs, i),
                                            ad_certs_get_priv_key_len(Certs, i));
                    OutCerts.Ok.Add(cert);
                }
                ad_certs_free(Certs);
            }
        }
    }

    void FetchApps(const AAPL::Team& Team, Result<TArray<App>>& OutApps)
    {
        if (m_Session) {
            const char* tid = TCHAR_TO_UTF8(*Team.ID);
            auto Apps = ads_fetch_apps(m_Session, tid, &m_Error);
            if (m_Error) {
                OutApps.Error = UTF8_TO_TCHAR(ads_err_get_msg(m_Error));
                OutApps.Code = ads_err_get_code(m_Error);
                ads_err_free(&m_Error);
                m_Error = nullptr;
            }
            if (Apps) {
                for (int32 i = 0; i < ad_apps_size(Apps); i++) {
                    AAPL::App app;
                    app.ID = UTF8_TO_TCHAR(ad_apps_get_id(Apps, i));
                    app.Name = UTF8_TO_TCHAR(ad_apps_get_name(Apps, i));
                    app.BundleID = UTF8_TO_TCHAR(ad_apps_get_bundle_id(Apps, i));
                    OutApps.Ok.Add(app);
                }
                ad_apps_free(Apps);
            }
        }
    }

private:
    ads_session* m_Session;
    ads_error m_Error;
    int m_NeedVerify;
};
} // AAPL
#endif // __UNREAL__

#endif

#endif