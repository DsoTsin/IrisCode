
namespace cpp iris

exception error {
    1: string       description,
    2: i16          code,
}

struct developer_account {
    1: string apple_id;
    2: string identifier;
    3: string first_name;
    4: string last_name;
    5: string status;
}

struct login_response {
    1: bool need_2factor_code;
    2: optional developer_account account;
}

// don't change !!!!
struct verify_response {
    1: bool succeed;
    2: optional developer_account account;
    3: optional string code;
}

enum developer_type {
    unknown,
    free,
    individual,
    organization
}

struct developer_team {
    1: string name;
    2: string id;
    3: developer_type type;
}

enum device_type {
    iphone = 1,
    ipad = 2,
    tv = 4,
    all = 7,
    none = 0
}

struct device {
    1: string name;
    2: string id;
    3: device_type type;
    // TODO: os ver
}

struct certificate {
    1: string name;
    2: string serial_no;
    3: string id;
    4: string machine_name;
    5: string machine_id;

    6: optional binary data;
    7: optional binary priv_key;
}

struct app_info {
    1: string name;
    2: string id;
    3: string bundle_id;
}

struct app_group_info {
    1: string name;
    2: string id;
    3: string group_id;
}

struct provision_profile {
    1: string name;
    2: optional string id;
    3: string uuid;
    // for app
    4: string bundle_id;
    5: string team_id;
    6: binary data;
    7: bool is_free;
    8: string type;
    9: string status;
}

service developer {
    login_response  login(1:string appleid, 2: string password) throws (1: error err),
    // 2 Factor verify and fetch account, don't change !!!!
    verify_response verify(1:string code)
    list<developer_team> fetch_team()

    // Devices
    list<device> fetch_devices(1: string team_id)
    void register_device(1: device dev, 2: string team_id)

    // Certificates
    list<certificate> fetch_certificates(1: string team_id)
    certificate add_certificate(1: string machine_name, 2: string team_id)
    bool revoke_certificate(1: string cert_id, 2: string team_id)

    // App IDs
    list<app_info> fetch_apps(1: string team_id)
    app_info add_app(1: string name, 2: string bundle_id, 3: string team_id)
    // update entitlement / features
    app_info update_app(1: app_info app, 2: string team_id)

    // App Groups
    list<app_group_info> fetch_app_groups(1: string team_id)
    app_group_info add_app_group(1: string group_name, 2: string group_id, 3: string team_id)
    bool assign_app_group(1: app_info app, 2: list<app_group_info> groups, 3: string team_id)

    // Provisioning Profiles
    provision_profile fetch_provisioning_profiles(1: string app_id, 2: device_type dty, 3: string team_id)
    bool delete_provisioning_profile(1: string provisioning_profile_id, 2: string team_id)
}