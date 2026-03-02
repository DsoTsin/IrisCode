use md5::{Md5, Digest};

const TYPE_MAP: &[u32;7] = &[0,1,4,3,2,5,6];

/// X-Mme-Device-Id
pub fn get_device_id() -> String {
    let mut values = Vec::new();
    for i in 0..7 {
        let mut hasher = Md5::new();
        if !get_machine_id(TYPE_MAP[i], false, &mut hasher) {
            values.push("00000000".into());
        } else {
            let digest = hasher.finalize();
            let code = uencode(&digest[..], 4, 32);
            let value = String::from_utf8(code).unwrap();
            values.push(value);
        }
    }
    values.join(".")
}

/// For mobile
/// X-Mme-Legacy-Device-Id
pub fn get_legacy_device_id() -> String {
    let mut result = String::new();
    let mut main_hasher = Md5::new();
    main_hasher.update("cache-control".as_bytes());
    main_hasher.update("Ethernet".as_bytes());
    for i in [1, 2, 3, 4] {
        let mut hasher = Md5::new();
        if !get_machine_id(i, true, &mut hasher) {
        } else {
            let digest = hasher.finalize();
            assert_eq!(digest.len(), 16);
            main_hasher.update(&digest[..4]); // 32 bits, 4 bytes
        }
    }
    let digest = main_hasher.finalize();
    for c in &digest[..] {
        result += format!("{:02x}", &c).as_str();
    }
    result += "00000000";
    result
}

fn get_machine_id(ty: u32, legacy: bool, md5: &mut Md5) -> bool {
    match ty {
        0 => {
            get_adapter_info(legacy, md5)
        },
        1 => {
            get_volume_info(md5)
        },
        2 => {
            get_bios_ver(legacy, md5)
        },
        3 => { // CPU string, passed
            get_cpu_string(legacy, md5)
        },
        4 => { // Product Id
            get_product_id(md5)
        },
        5 => { // Computer Name, passed
            get_computer_name(md5);
            true
        },
        6 => { // HwProfile, passed
            let bytes = get_hwprof();
            md5.update(&bytes);
            true
        }
        _ => false
    }
}

fn get_adapter_info(legacy: bool, md5: &mut Md5) -> bool {
    use winapi::um::iphlpapi::*;
    use winapi::um::heapapi::*;
    use winapi::um::iptypes::*;
    let mut count = 0;
    let mut bytes = Vec::new();
    bytes.resize(6, 0);
    if legacy {
        unsafe { 
            GetAdaptersInfo(std::ptr::null_mut(), &mut count); 
            let data = HeapAlloc(GetProcessHeap(), 0, count as _);
            GetAdaptersInfo(data.cast(), &mut count);
            let info: winapi::um::iptypes::PIP_ADAPTER_INFO = data as _;
            libc::memcpy(bytes.as_mut_ptr() as _, (*info).Address.as_ptr() as _, 6);
            md5.update(&bytes[..6]);
            HeapFree(GetProcessHeap(), 0, data);
        }
    } else {
        use winapi::um::winnt::*;
        use winapi::um::winbase::*;
        use winapi::shared::ntdef::*;
        unsafe {
            let mut status = 111;
            let mut verinfo: OSVERSIONINFOEXW = std::mem::zeroed();
            verinfo.dwOSVersionInfoSize = 284;
            let a = VerSetConditionMask(0, 2, 3);
            let b = VerSetConditionMask(a, 1, 3);
            let c = VerSetConditionMask(b, 0x20, 3);
            verinfo.dwMajorVersion = 6;
            verinfo.dwMinorVersion = 2;
            verinfo.wServicePackMajor = /*SP major*/0;
            if VerifyVersionInfoW(&mut verinfo, 0x23, c) == 1 {
                let mut adapter_addrs: PIP_ADAPTER_ADDRESSES = std::ptr::null_mut();
                let mut size_ptr: ULONG = 12;
                while status == 111 {
                    if !adapter_addrs.is_null() {
                        HeapFree(GetProcessHeap(),  0, adapter_addrs as _);
                    }
                    adapter_addrs = HeapAlloc(GetProcessHeap(), 0, size_ptr as _) as _;
                    if adapter_addrs.is_null() {
                        break;
                    }
                    status = GetAdaptersAddresses(0, 0x100, std::ptr::null_mut(), adapter_addrs, &mut size_ptr);
                }
                if 0 == status {
                    for i in 0..(*adapter_addrs).PhysicalAddressLength as usize {
                        if i < 6 {
                            let a = (*adapter_addrs).PhysicalAddress[i];
                            bytes[i] = a;
                        } else {
                            break;
                        }
                    }
                }
                if !adapter_addrs.is_null() {
                    HeapFree(GetProcessHeap(),  0, adapter_addrs as _);
                }
            } else {
                let mut size_ptr = 1944;
                let mut adapter_info: PIP_ADAPTER_INFO = std::ptr::null_mut();
                while status == 111 {
                    if !adapter_info.is_null() {
                        HeapFree(GetProcessHeap(),  0, adapter_info as _);
                    }
                    adapter_info = HeapAlloc(GetProcessHeap(), 0, size_ptr as _) as _;
                    if adapter_info.is_null() {
                        break;
                    }
                    status = GetAdaptersInfo(adapter_info, &mut size_ptr);
                }
                if status == 0 {
                    for i in 0..(*adapter_info).AddressLength as usize {
                        if i < 6 {
                            bytes[i] = (*adapter_info).Address[i];
                        } else {
                            break;
                        }
                    }
                }
                if !adapter_info.is_null() {
                    HeapFree(GetProcessHeap(),  0, adapter_info as _);
                }
            }
            md5.update(&bytes[..6]);
        }
    }
    true
}

fn get_volume_info(md5: &mut Md5) -> bool {
    use winapi::um::fileapi::*;
    let mut serial = 0; // 4bytes
    let str: Vec<_> = "C:\\\0".encode_utf16().collect();
    if unsafe { GetVolumeInformationW(str.as_ptr() , std::ptr::null_mut(), 0, &mut serial, 
            std::ptr::null_mut(), std::ptr::null_mut(), std::ptr::null_mut(), 0) 
        } != 0 {
        md5.update(serial.to_le_bytes());
        return true;
    }
    false
}

fn get_bios_ver(legacy: bool, md5: &mut Md5) -> bool {
    use winapi::um::winreg::*;
    unsafe {
        let mut key = std::mem::zeroed();
        RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\0".as_ptr() as _, 0, 0x20019u32, &mut key);
        if key.is_null() {
            return false;
        }
        let mut count = 0;
        let mut bytes = Vec::new();
        if legacy {
            let str:Vec<_> = "SystemBiosVersion\0".encode_utf16().collect();
            if 0 == RegQueryValueExW(key, str.as_ptr(), std::ptr::null_mut(), std::ptr::null_mut(), std::ptr::null_mut(), &mut count) {
                bytes.resize(count as _, 0);
                RegQueryValueExW(key, str.as_ptr(), std::ptr::null_mut(), std::ptr::null_mut(), bytes.as_mut_ptr(), &mut count);
            }
        } else {
            if 0 == RegQueryValueExA(key, "SystemBiosVersion\0".as_ptr() as _, std::ptr::null_mut(), std::ptr::null_mut(), std::ptr::null_mut(), &mut count)  {
                bytes.resize(count as _, 0);
                RegQueryValueExA(key, "SystemBiosVersion\0".as_ptr() as _, std::ptr::null_mut(), std::ptr::null_mut(), bytes.as_mut_ptr(), &mut count);
            }
        }
        RegCloseKey(key);
        if count > 0 {
            md5.update(&bytes[..count as usize]);
        }
        count > 0
    }
}

fn get_cpu_string(legacy: bool, md5: &mut Md5) -> bool {
    use winapi::um::winreg::*;
    unsafe {
        let mut key = std::mem::zeroed();
        RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0\0".as_ptr() as _, 0, 0x20019u32, &mut key);
        let mut count = 0;
        let mut bytes = Vec::new();
        if legacy {
            let str:Vec<_> = "ProcessorNameString\0".encode_utf16().collect();
            if 0 == RegQueryValueExW(key, str.as_ptr(), std::ptr::null_mut(), std::ptr::null_mut(), std::ptr::null_mut(), &mut count) {
                bytes.resize(count as _, 0);
                RegQueryValueExW(key, str.as_ptr(), std::ptr::null_mut(), std::ptr::null_mut(), bytes.as_mut_ptr(), &mut count);
            }
        } else {
            if 0 == RegQueryValueExA(key, "ProcessorNameString\0".as_ptr() as _, std::ptr::null_mut(), std::ptr::null_mut(), std::ptr::null_mut(), &mut count) {
                bytes.resize(count as _, 0);
                RegQueryValueExA(key, "ProcessorNameString\0".as_ptr() as _, std::ptr::null_mut(), std::ptr::null_mut(), bytes.as_mut_ptr(), &mut count);
            }
        }
        RegCloseKey(key);
        if count > 0 {
            md5.update(&bytes[..count as _]);
        }
        count > 0
    }
}

fn get_product_id(md5: &mut Md5) -> bool  {
    use winapi::um::winreg::*;
    unsafe {
        let mut key = std::mem::zeroed();
        RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\0".as_ptr() as _, 0, 0x20019u32, &mut key);
        if key.is_null() {
            return false;
        }
        let mut count = 0;
        let mut bytes = Vec::new();
        if 0 == RegQueryValueExA(key, "ProductId\0".as_ptr() as _, std::ptr::null_mut(), std::ptr::null_mut(), std::ptr::null_mut(), &mut count) {
            bytes.resize(count as _, 0);
            RegQueryValueExA(key, "ProductId\0".as_ptr() as _, std::ptr::null_mut(), std::ptr::null_mut(), bytes.as_mut_ptr(), &mut count);
            md5.update(&bytes[..count as _]);
        }
        RegCloseKey(key);
        count > 0
    }
}

fn get_computer_name(md5: &mut Md5) {
    use winapi::um::winbase::*;
    let mut buffer = Vec::new();
    let mut len: winapi::shared::minwindef::DWORD = 16;
    buffer.resize(len as usize * 2, 0u8);
    unsafe { GetComputerNameW(buffer.as_mut_ptr() as _, &mut len) };
    md5.update(&buffer[..len as _]);
}

fn get_hwprof() -> Vec<u8> {
    use winapi::um::winbase::*;
    unsafe {
        let mut prof = std::mem::zeroed::<HW_PROFILE_INFOW>();
        GetCurrentHwProfileW(&mut prof);
        let len = libc::wcslen(prof.szHwProfileGuid.as_ptr() as _);
        let mut bytes = Vec::new();
        bytes.resize(len * 2, 0);
        libc::memcpy(bytes.as_mut_ptr() as _, prof.szHwProfileGuid.as_ptr() as _, len * 2);
        bytes
    }
}

#[cfg(windows)]
#[link(name = "shlwapi")]
extern "stdcall" {
    fn PathRemoveBackslashW(pszpath: *mut u16) -> *mut u16;
    fn PathFindNextComponentW(pszpath: *mut u16) -> *mut u16;
}

/// X-Apple-I-MD-LU
pub fn get_local_user_id() -> String {
    use winapi::um::processthreadsapi::{GetCurrentProcess, OpenProcessToken};
    use winapi::um::userenv::GetUserProfileDirectoryW;
    use winapi::um::handleapi::CloseHandle;
    unsafe {
        let mut token = std::mem::zeroed();
        let proc = GetCurrentProcess();
        if 0 == OpenProcessToken(proc, 0x20008u32, &mut token) {
            return "".into();
        }
        let mut len = 261;
        let mut dir: [u16;262] = [0;262];
        if 0 == GetUserProfileDirectoryW(token, dir.as_mut_ptr() as _, &mut len) || len == 0 {
            CloseHandle(token);
            return "".into();
        }
        let mut result = String::new();
        if len > 0 {
            PathRemoveBackslashW(dir.as_mut_ptr());
            let mut new_dir = dir.as_mut_ptr();
            let mut last_comp = std::ptr::null_mut();
            let mut last_len = 0;
            while !new_dir.is_null() {
                let len = libc::wcslen(new_dir as _);
                if len > 0 {
                    last_comp = new_dir;
                    last_len = len;
                }
                new_dir = PathFindNextComponentW(new_dir);
            }
            if !last_comp.is_null() {
                let mut md5 = Md5::new();
                let data = std::slice::from_raw_parts_mut(last_comp as *mut u8, last_len * 2);
                md5.update(data);
                let digest = md5.finalize();
                let encoded = uencode(&digest[..], 16, 65);
                result = String::from_utf8(encoded).unwrap();
            }
        }
        CloseHandle(token);

        result
    }
}

const CODES: &[char;16] = &['0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'];
fn uencode(bytes:&[u8], inbytes: usize, bits: usize) -> Vec<u8> {
    assert!(bits > 2 * inbytes);
    let mut result = Vec::new();
    for i in 0..inbytes {
        let byte = bytes[i];
        result.push(CODES[(byte >> 4) as usize] as u8);
        result.push(CODES[(byte & 0xf) as usize] as u8);
    }
    result
}

#[cfg(test)]
mod tests {
    use super::*;
    /*
        UDID: <04B8EDAF.CB374885.00000000.54B38136.20BA4F1E.AB7F7EB4.310A3EFB>
        Legacy UDID: <b1d05c4fabd312e05c3e45e0da1d66e500000000>
        Local UUID: <9723730C5781E66E907720158FB8CF2C>
    */
    #[test]
    fn test_udid() {
        let result = get_device_id();
        //println!("expect: 04B8EDAF.CB374885.00000000.54B38136.20BA4F1E.AB7F7EB4.310A3EFB");
        println!("expt64: 04B8EDAF.CB374885.2D733D0C.54B38136.20BA4F1E.AB7F7EB4.310A3EFB");
        println!("result: {}", result);
    }

    #[test]
    fn test_legacy_udid() {
        let result = get_legacy_device_id();
        //println!("expect: b1d05c4fabd312e05c3e45e0da1d66e500000000"); // 32 bits
        println!("expt64: 049d2ac8a121c1aa597f17abceff234c00000000"); // 64 bits not the same
        println!("result: {}", result);
    }

    #[test]
    fn test_local_uuid() {
        let result = get_local_user_id();
        println!("expect: 9723730C5781E66E907720158FB8CF2C");
        println!("result: {}", result);
    }
}