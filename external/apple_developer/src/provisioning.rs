
use std::sync::Arc;
use serde::{
    Serialize,
    Deserialize
};
use crate::{
    Result,
    machine::{
        get_device_id, 
        get_legacy_device_id, 
        get_local_user_id
    }
};

pub fn update_machine_provisioning() -> Result<()> {
    let _ret = gsa_lookup()?;
    let rsp = gs_start_machine_provision()?;
    println!("startProvision: {:#?}", rsp);
    gs_finish_machine_provision(rsp.response.spim)
}

mod apple {
    use super::*;
    
    #[derive(Deserialize, Debug)]
    pub struct AIDCConfigs {
        #[serde(rename = "threshold-getMyInfo")]
        pub get_my_info: i64,
        #[serde(rename = "threshold-certCertificate")]
        pub cert_cert: i64,
        #[serde(rename = "threshold-fetchCertificate")]
        pub fetch_cert: i64,
        #[serde(rename = "threshold-findPerson")]
        pub find_person: i64,
    }
    
    #[derive(Deserialize, Debug)]
    pub struct Configs {
        #[serde(rename = "appleOwnedDomains")]
        pub apple_owned_domains: Vec<String>,
        #[serde(rename = "appleIDAuthorizationUrls")]
        pub apple_id_auth_urls: Vec<String>,
        #[serde(rename = "2faUpgradeAccountTypePriority")]
        pub two_factor_upgrade_account_type_prio: Vec<String>,
        #[serde(rename = "is-in-line-flow-supported")]
        pub is_in_line_flow_supported: bool,
        #[serde(rename = "is-phone-number-supported")]
        pub is_phone_number_supported: bool,
        #[serde(rename = "abs-enable")]
        pub abs_enable: i64,
        #[serde(rename = "aidc-cfgs")]
        pub aidc_cfgs: AIDCConfigs,
    }
    
    #[derive(Deserialize, Debug)]
    pub struct Env {
        #[serde(rename = "apsEnv")]
        pub aps_env: String,
        #[serde(rename = "idmsEnv")]
        pub idms_env: String,
    }

    #[derive(Deserialize, Debug)]
    pub struct LookupResponse {
        pub cfgs: Configs,
        pub env: Env,
        pub urls: plist::Value,
    }

    #[derive(Deserialize, Debug)]
    pub struct MachineProvisionStatus {
        pub ed: String,
        pub ec: i64,
        pub em: String,
    }
    
    #[derive(Deserialize, Debug)]
    pub struct MachineProvisionResponse {
        #[serde(rename = "Status")]
        pub status: MachineProvisionStatus,
        pub spim: String,
    }

    #[derive(Deserialize, Debug)]
    pub struct StartMachineProvisioningResponse {
        #[serde(rename = "Response")]
        pub response: MachineProvisionResponse,
        #[serde(rename = "Header")]
        pub header: plist::Value,
    }

    #[derive(Serialize, Debug)]
    pub struct CPIM {
        pub cpim: String,
    }

    #[derive(Serialize, Debug)]
    pub struct FinishProvisionHeader {
    }
    
    #[derive(Serialize, Debug)]
    pub struct FinishMachineProvisioningContent {
        #[serde(rename = "Request")]
        pub request: CPIM,
        #[serde(rename = "Header")]
        pub header: FinishProvisionHeader,
    }
}

fn gsa_lookup() -> Result<apple::LookupResponse> {
    let agent = ureq::AgentBuilder::new()
        .tls_config(Arc::new(danger::new_tls_config()))
        .build();    
    let result = agent.get("https://gsa.apple.com/grandslam/GsService2/lookup")
    .set("X-Mme-Client-Info", "<PC> <Windows;6.2(0,0);9200> <com.apple.AuthKitWin/1 (com.apple.iCloud/1)>")
    .set("X-Mme-Device-Id", &get_device_id())
    .set("X-Mme-Legacy-Device-Id", &get_legacy_device_id())
    .set("X-Apple-I-MD-LU", &get_local_user_id())
    .set("Accept-Language", "en")
    .set("X-Mme-Country", "US")
    .call()?
    .into_reader();
    let result: apple::LookupResponse = plist::from_reader_xml(result)?;
    Ok(result)
}

fn gs_start_machine_provision() -> Result<apple::StartMachineProvisioningResponse> {
    let agent = ureq::AgentBuilder::new()
        .tls_config(Arc::new(danger::new_tls_config()))
        .build();    
    let reader = agent.post("https://gsa.apple.com/grandslam/MidService/startMachineProvisioning")
    .set("X-Mme-Client-Info", "<PC> <Windows;6.2(0,0);9200> <com.apple.AuthKitWin/1 (com.apple.iCloud/1)>")
    .set("X-Mme-Device-Id", &get_device_id())
    .set("X-Mme-Legacy-Device-Id", &get_legacy_device_id())
    .set("X-Apple-I-MD-LU", &get_local_user_id())
    .set("Accept-Language", "en")
    .set("X-Mme-Country", "US")
    .set("Content-Type", "text/x-xml-plist")
    .send_string("")?
    .into_reader();
    let response: apple::StartMachineProvisioningResponse = plist::from_reader_xml(reader)?;
    Ok(response)
}

fn gs_finish_machine_provision(cpim: String) -> Result<()> {
    // let len = unsafe { encode(cpim.as_ptr(), cpim.len(), std::ptr::null_mut(), 0) };
    // let mut bytes = Vec::new();
    // bytes.resize(len, 0u8);
    // unsafe { encode(cpim.as_ptr(), cpim.len(), bytes.as_mut_ptr(), len) };
    // let dsid = -2;
    // let cpim = base64::encode(bytes);
    let content = apple::FinishMachineProvisioningContent{
        header: apple::FinishProvisionHeader{},
        request: apple::CPIM { cpim }
    };
    let mut buf = Vec::new();
    plist::to_writer_xml(&mut buf, &content)?;
    let str = String::from_utf8(buf).unwrap();
    println!("{}", &str);
    let agent = ureq::AgentBuilder::new()
        .tls_config(Arc::new(danger::new_tls_config()))
        .build();
    let body: String = agent.post("https://gsa.apple.com/grandslam/MidService/finishMachineProvisioning")
    .set("X-Mme-Client-Info", "<PC> <Windows;6.2(0,0);9200> <com.apple.AuthKitWin/1 (com.apple.iCloud/1)>")
    .set("X-Mme-Device-Id", &get_device_id())
    .set("X-Mme-Legacy-Device-Id", &get_legacy_device_id())
    .set("X-Apple-I-MD-LU", &get_local_user_id())
    .set("Accept-Language", "en")
    .set("X-Mme-Country", "US")
    .set("Content-Type", "text/x-xml-plist")
    .send_string(&str)?
    .into_string()?;
    println!("BODY: {}", body);
    Ok(())
}

mod danger {
    pub struct NoCertificateVerification {}

    impl rustls::client::ServerCertVerifier for NoCertificateVerification {
        fn verify_server_cert(
            &self,
            _end_entity: &rustls::Certificate,
            _intermediates: &[rustls::Certificate],
            _server_name: &rustls::ServerName,
            _scts: &mut dyn Iterator<Item = &[u8]>,
            _ocsp: &[u8],
            _now: std::time::SystemTime,
        ) -> std::result::Result<rustls::client::ServerCertVerified, rustls::Error> {
            Ok(rustls::client::ServerCertVerified::assertion())
        }

        fn verify_tls12_signature(
            &self,
            _message: &[u8],
            _cert: &rustls::Certificate,
            _dss: &rustls::internal::msgs::handshake::DigitallySignedStruct,
        ) -> Result<rustls::client::HandshakeSignatureValid, rustls::Error> {
            Ok(rustls::client::HandshakeSignatureValid::assertion())
        }

        fn verify_tls13_signature(
            &self,
            _message: &[u8],
            _cert: &rustls::Certificate,
            _dss: &rustls::internal::msgs::handshake::DigitallySignedStruct,
        ) -> Result<rustls::client::HandshakeSignatureValid, rustls::Error> {
            Ok(rustls::client::HandshakeSignatureValid::assertion())
        }
    }

    pub fn new_tls_config() -> rustls::ClientConfig {
        let root_store = rustls::RootCertStore::empty();
        let protocol_versions = &[&rustls::version::TLS12, &rustls::version::TLS13];
        let mut tls_config = rustls::ClientConfig::builder()
            .with_safe_default_cipher_suites()
            .with_safe_default_kx_groups()
            .with_protocol_versions(protocol_versions)
            .unwrap()
            .with_root_certificates(root_store)
            .with_no_client_auth();
        tls_config.dangerous().set_certificate_verifier(std::sync::Arc::new(NoCertificateVerification {}));
        tls_config
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_anisette() -> Result<()> {
        update_machine_provisioning()
    }
}