#include "apple_developer_service_api.h"
#include "developer.h"
#include "developer_api.h"
#include "exception.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TPipe.h>
#include <thrift/transport/TServerSocket.h>

#include <memory>

using namespace std;
using namespace iris;

#define CLEAN_ERROR(error)                                                     \
  if (error) {                                                                 \
    if (*error) {                                                              \
      delete (::apache::thrift::TException *)*error;                                            \
    }                                                                          \
    *error = nullptr;                                                          \
  }

#define STR_ATTR_GET_ARRAY(type, cast_type, attr)                              \
  const char *ad_##type##_get_##attr(ad_##type _##type, int index) {           \
    if (_##type) {                                                             \
      auto real = (cast_type *)_##type;                                        \
      return real->at(index).##attr##.c_str();                                 \
    }                                                                          \
    return nullptr;                                                            \
  }

#define DEF_ARRAY(type, cast_type)                                             \
  void ad_##type##_free(ad_##type _##type) {                                   \
    if (_##type) {                                                             \
      auto real = (cast_type *)_##type;                                        \
      delete real;                                                             \
    }                                                                          \
  }                                                                            \
  int ad_##type##_size(ad_##type _##type) {                                    \
    if (_##type) {                                                             \
      auto real = (cast_type *)_##type;                                        \
      return (int)real->size();                                                \
    }                                                                          \
    return -1;                                                                 \
  }

class SessionImpl : public iris::developerClient {
public:
  SessionImpl() : developerClient(nullptr) {
    pipe_ = make_shared<apache::thrift::transport::TPipe>(API_ENDPOINT);
    // TODO: openssl
    auto protocol = make_shared<apache::thrift::protocol::TBinaryProtocol>(
        make_shared<apache::thrift::transport::TBufferedTransport>(pipe_));
    piprot_ = protocol;
    poprot_ = protocol;
    iprot_ = protocol.get();
    oprot_ = protocol.get();
    pipe_->open();
  }
  ~SessionImpl() override {
    if (pipe_) {
      pipe_->close();
    }
  }
  void login2(login_response &_return, const string &appleid,
              const string &password,
              const function<void(string &code)> &verify_callback);

private:
  shared_ptr<apache::thrift::transport::TPipe> pipe_;
};

const char *ads_err_get_msg(ads_error err) {
  auto ex = (::apache::thrift::TException *)err;
  return ex->what();
}

int ads_err_get_code(ads_error err) { 
  auto ex = (::apache::thrift::TException *)err;
  return ex->kind();    
}

void ads_err_free(ads_error *err) {
  if (err && *err) {
    auto strerr = (::apache::thrift::TException *)*err;
    delete strerr;
    *err = nullptr;
  }
}

ads_session *ads_login2(const char *apple_id, const char *password,
                        verify_callback callback, void *usr_data,
                        ads_error *error) {
  CLEAN_ERROR(error)
  iris::login_response rsp;
  try {
    auto session = new SessionImpl;
    session->login2(rsp, apple_id, password,
                    [=](string &code) { code = callback(usr_data); });

    //if (rsp.)

    return (ads_session *)session;
  } catch (const ::iris::error &e) {
    if (error) {
      *error =
          (ads_error) new ::apache::thrift::TException(e.description, e.code);
    }
    return nullptr;
  } catch (const apache::thrift::TException &e) {
    if (error) {
      *error = (ads_error) new ::apache::thrift::TException(e);
    }
    return nullptr;
  }
}

void ads_free(ads_session *session) {
  if (session) {
    auto s = (SessionImpl *)session;
    delete s;
  }
}

DEF_ARRAY(teams, vector<developer_team>)
STR_ATTR_GET_ARRAY(teams, vector<developer_team>, id)
STR_ATTR_GET_ARRAY(teams, vector<developer_team>, name)

ad_teams ads_fetch_teams(ads_session *session, ads_error *error) {
  CLEAN_ERROR(error)
  auto s = (SessionImpl *)session;
  vector<iris::developer_team> teams;
  try {
    s->fetch_team(teams);
    return new vector<iris::developer_team>(std::move(teams));
  } catch (const apache::thrift::TException &e) {
    if (error) {
      *error = (ads_error) new ::apache::thrift::TException(e);
    }
    return nullptr;
  }
}

DEF_ARRAY(devices, vector<device>)
STR_ATTR_GET_ARRAY(devices, vector<device>, id)
STR_ATTR_GET_ARRAY(devices, vector<device>, name)

ad_devices ads_fetch_devices(ads_session *session, const char *team_id,
                             ads_error *error) {
  CLEAN_ERROR(error)
  auto s = (SessionImpl *)session;
  vector<iris::device> devs;
  try {
    s->fetch_devices(devs, team_id);
    return new std::vector<iris::device>(std::move(devs));
  } catch (const apache::thrift::TException &e) {
    if (error) {
      *error = (ads_error) new ::apache::thrift::TException(e);
    }
    return nullptr;
  }
}

ad_device ads_register_device(ads_session *session, const char *team_id,
                              const char *mach_name, const char *mach_id,
                              ads_device_type dtype, ads_error *error) {
  CLEAN_ERROR(error)
  auto s = (SessionImpl *)session;
  try {
    iris::device dev{};
    dev.__set_name(mach_name);
    dev.__set_id(mach_id);
    dev.__set_type((iris::device_type::type)dtype);
    s->register_device(dev, team_id);
    return nullptr;
  } catch (const apache::thrift::TException &e) {
    if (error) {
      *error = (ads_error) new ::apache::thrift::TException(e);
    }
    return nullptr;
  }
}

DEF_ARRAY(certs, vector<certificate>)
STR_ATTR_GET_ARRAY(certs, vector<certificate>, id)
STR_ATTR_GET_ARRAY(certs, vector<certificate>, name)
STR_ATTR_GET_ARRAY(certs, vector<certificate>, serial_no)

// certificate/downloadCertificateContent.action
// ios/certificate/downloadCertificateContent.action
ad_certs ads_fetch_certificates(ads_session *session, const char *team_id,
                                ads_error *error) {
  CLEAN_ERROR(error)
  auto s = (SessionImpl *)session;
  vector<certificate> certs;
  try {
    s->fetch_certificates(certs, team_id);
    return new vector<certificate>(move(certs));
  } catch (const apache::thrift::TException &e) {
    if (error) {
      *error = (ads_error) new ::apache::thrift::TException(e);
    }
    return nullptr;
  }
}

ad_cert ads_add_certificates(ads_session *session, const char *team_id,
                             const char *mach_name, ads_error *error) {
  CLEAN_ERROR(error)
  auto s = (SessionImpl *)session;
  certificate rsp;
  try {
    s->add_certificate(rsp, mach_name, team_id);
    return (ad_cert) new certificate(move(rsp));
  } catch (const apache::thrift::TException &e) {
    if (error) {
      *error = (ads_error) new ::apache::thrift::TException(e);
    }
    return nullptr;
  }
}

int ads_revoke_certificate(ads_session *session, const char *team_id,
                           const char *cert_id, ads_error *error) {
  CLEAN_ERROR(error)
  auto s = (SessionImpl *)session;
  try {
    return s->revoke_certificate(cert_id, team_id) ? 1 : 0;
  } catch (const apache::thrift::TException &e) {
    if (error) {
      *error = (ads_error) new ::apache::thrift::TException(e);
    }
    return 0;
  }
}

DEF_ARRAY(apps, vector<app_info>)
STR_ATTR_GET_ARRAY(apps, vector<app_info>, id)
STR_ATTR_GET_ARRAY(apps, vector<app_info>, name)
STR_ATTR_GET_ARRAY(apps, vector<app_info>, bundle_id)

ad_apps ads_fetch_apps(ads_session *session, const char *team_id,
                       ads_error *error) {
  CLEAN_ERROR(error)
  auto s = (SessionImpl *)session;
  vector<app_info> rsp;
  try {
    s->fetch_apps(rsp, team_id);
    return (ad_apps) new vector<app_info>(move(rsp));
  } catch (const apache::thrift::TException &e) {
    if (error) {
      *error = (ads_error) new ::apache::thrift::TException(e);
    }
    return nullptr;
  }
}

ad_app_groups ads_fetch_app_groups(ads_session *session, const char *team_id,
                                   ads_error *error) {
  CLEAN_ERROR(error)
  auto s = (SessionImpl *)session;
  vector<app_group_info> rsp;
  try {
    s->fetch_app_groups(rsp, team_id);
    return (ad_apps) new vector<app_group_info>(move(rsp));
  } catch (const apache::thrift::TException &e) {
    if (error) {
      *error = (ads_error) new ::apache::thrift::TException(e);
    }
    return nullptr;
  }
}

void ad_provision_profile_free(ad_provisioning_profile profile) {
  if (profile) {
    provision_profile *prof = (provision_profile *)profile;
    delete prof;
  }
}
const char *ad_provision_profile_get_id(ad_provisioning_profile profile) {
  if (profile) {
    provision_profile *prof = (provision_profile *)profile;
    return prof->id.c_str();
  }
  return nullptr;
}
const char *ad_provision_profile_get_name(ad_provisioning_profile profile) {
  if (profile) {
    provision_profile *prof = (provision_profile *)profile;
    return prof->name.c_str();
  }
  return nullptr;
}
const char *ad_provision_profile_get_type(ad_provisioning_profile profile) {
  if (profile) {
    provision_profile *prof = (provision_profile *)profile;
    return prof->type.c_str();
  }
  return nullptr;
}
const char *
ad_provision_profile_get_bundle_id(ad_provisioning_profile profile) {
  if (profile) {
    provision_profile *prof = (provision_profile *)profile;
    return prof->bundle_id.c_str();
  }
  return nullptr;
}
const char *ad_provision_profile_data(ad_provisioning_profile profile) {
  if (profile) {
    provision_profile *prof = (provision_profile *)profile;
    return prof->data.c_str();
  }
  return nullptr;
}
int ad_provision_profile_size(ad_provisioning_profile profile) {
  if (profile) {
    provision_profile *prof = (provision_profile *)profile;
    return (int)prof->data.size();
  }
  return -1;
}

ad_provisioning_profile ads_fetch_provision_profile(ads_session *session,
                                                    const char *team_id,
                                                    const char *app_id,
                                                    ads_device_type dtype,
                                                    ads_error *error) {
  CLEAN_ERROR(error)
  auto s = (SessionImpl *)session;
  provision_profile profile;
  try {
    s->fetch_provisioning_profiles(profile, app_id, (device_type::type)dtype,
                                   team_id);
    return (ad_provisioning_profile) new provision_profile(move(profile));
  } catch (const apache::thrift::TException &e) {
    if (error) {
      *error = (ads_error) new ::apache::thrift::TException(e);
    }
    return nullptr;
  }
}

void SessionImpl::login2(login_response &_return, const string &appleid,
                         const string &password,
                         const function<void(string &code)> &verify_callback) {
  send_login(appleid, password);

  int32_t rseqid = 0;
  std::string fname;
  ::apache::thrift::protocol::TMessageType mtype;
  iprot_->readMessageBegin(fname, mtype, rseqid);

  if (mtype == ::apache::thrift::protocol::T_EXCEPTION) {
    ::apache::thrift::TApplicationException x;
    x.read(iprot_);
    iprot_->readMessageEnd();
    iprot_->getTransport()->readEnd();
    throw x;
  }

  switch (mtype) {
  case ::apache::thrift::protocol::T_CALL: {
    if (fname.compare("verify") == 0) {
      std::string sname;
      ::apache::thrift::protocol::TType ftype;
      int16_t fid = 0;
      uint32_t xfer = 0;
      xfer += iprot_->readStructBegin(sname);
      iprot_->readFieldBegin(fname, ftype, fid);
      string code;
      iprot_->readString(code);
      iprot_->readFieldEnd();
      iprot_->readFieldBegin(fname, ftype, fid);
      iprot_->readStructEnd();
      iprot_->readMessageEnd();
      iprot_->getTransport()->readEnd();

      if (code == "request_verify") {
        verify_callback(code);
        int32_t rseqid = 0;
        oprot_->writeMessageBegin("verify", ::apache::thrift::protocol::T_REPLY,
                                  rseqid);
        {
          developer_verify_result result;
          verify_response rsp;
          rsp.__set_code(code);
          rsp.__set_succeed(true);
          result.success = (rsp);
          result.__isset.success = true;
          result.write(oprot_);
        }
        oprot_->writeMessageEnd();
        oprot_->getTransport()->writeEnd();
        oprot_->getTransport()->flush();
      }

      recv_login(_return);
    }
    break;
  }
  case ::apache::thrift::protocol::T_REPLY: {
    developer_login_presult result;
    result.success = &_return;
    result.read(iprot_);
    iprot_->readMessageEnd();
    iprot_->getTransport()->readEnd();
    if (result.__isset.success) {
      // _return pointer has now been filled
      return;
    }
    if (result.__isset.err) {
      throw result.err;
    }
    break;
  }
  default:
    if (mtype != ::apache::thrift::protocol::T_REPLY) {
      iprot_->skip(::apache::thrift::protocol::T_STRUCT);
      iprot_->readMessageEnd();
      iprot_->getTransport()->readEnd();
    }
    throw ::apache::thrift::TApplicationException(
        ::apache::thrift::TApplicationException::MISSING_RESULT,
        "login failed: unknown result, expected are verify or login");
    break;
  }
}