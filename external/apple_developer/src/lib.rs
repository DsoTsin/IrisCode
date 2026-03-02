/*
  How to run
    v15 = CFURLConnectionCreateWithProperties(kCFAllocatorDefault, a2, v17, dword_103905FC);
    Current = CFRunLoopGetCurrent(kCFRunLoopDefaultMode);
    CFURLConnectionScheduleWithRunLoop(v15, Current);
    CFURLConnectionStart(v15);
    CFRunLoopRun();
    v8 = _CFStringMakeConstantString("Finished http request (done=%d, success=%d, error=%@)");

GSClientIdentifier: AuthKit

Header:
    X-Mme-Client-Info:  <= UA / User-Agent + X-GS-Client-Info
    X-Mme-Device-Id:
    X-Mme-Legacy-Device-Id:
    X-Apple-I-MD-LU:
    Accept-Language: en <= AKUILanguage
    X-Mme-Country: US <= AKCountryCode

GetFromDict
    AKClientID: 
    AKClientVersion:
    AKCountryCode:
    AKUILanguage:


UA: <PC> <%@> <com.apple.AuthKitWin/1 (%@/%@)>

# References
    https://github.com/ionescu007/Blackwood-4NT
    https://vtky.github.io/2020/05/19/appleid-auth-part-1
    https://sector.ca/wp-content/uploads/presentations17/vladimir-Katalov-iCloud_Keychain-(Katalov).pdf

https://gsa.apple.com/grandslam/GsService2/lookup (iCloud, IdMS, Production)
- https://grandslam-uat.apple.com/grandslam/GsService2/lookup (iTunes, IdMS1, User Acceptance Testing (UAT))
- https://grandslam-it.apple.com/grandslam/GsService2/lookup (IdMS2, QA)
- https://grandslam-it3.apple.com/grandslam/GsService2/lookup (IdMS3, QA2)

midStartProvisioning
midFinishProvisioning
midSyncMachine

CopyAnisetteDataForMachine
    GetActiveAnisetteDSID
        GetIDMSBag

    _PerformAnisetteProvisioningForMachine
        GetActiveAnisetteDSID
        CreateStartProvisioningRequest (sp request)
            GetIDMSBag => URL
        CreateEndProvisioningRequest (ep request)
            => X_Apple_I_MD_RINFO
            => tk
            => ptm
    
    => X-Apple-I-MD
    => X-Apple-I-MD-M

Attempt to get r 
Attempt to get auth info


1. Lookup 
    https://gsa.apple.com/grandslam/GsService2/lookup
    - https://grandslam-it.apple.com/grandslam/GsService2/lookup
2. Start provisioning
    https://gsa.apple.com/grandslam/MidService/startMachineProvisioning (POST)
3: End provisioning
    https://gsa.apple.com/grandslam/MidService/finishMachineProvisioning (POST)
*/
mod machine;
mod provisioning;
mod anisette;
mod auth;
mod crypto;

extern crate plist;

#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("IO error: {0}")]
    IoError(#[from] std::io::Error),
    
    #[error("Http error: {0}")]
    HttpError(#[from] ureq::Error),

    #[error("Plist error: {0}")]
    PlistError(#[from] plist::Error),
}

pub type Result<T> = std::result::Result<T, Error>;

pub use provisioning::update_machine_provisioning;
pub use anisette::{acquire_anisette_v1, acquire_anisette_v2};
