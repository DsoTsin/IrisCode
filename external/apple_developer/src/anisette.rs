use winapi::shared::minwindef::FARPROC;

fn to_wide_chars(s: &str) -> Vec<u16> {
    use std::ffi::OsStr;
    use std::os::windows::ffi::OsStrExt;
    OsStr::new(s).encode_wide().chain(Some(0).into_iter()).collect::<Vec<_>>()
}

pub type FnGetClass = unsafe extern "C" fn(*const u8) -> *const libc::c_void;
pub type FnSelRegisterName = unsafe extern "C" fn(*const u8) -> *const libc::c_void;
pub type FnMsgSend = unsafe extern "C" fn(*const libc::c_void,*const libc::c_void) -> *const libc::c_void;
pub type FnCFRelease = unsafe extern "C" fn(*const libc::c_void);
pub type FnCopyAnisette = unsafe extern "C" fn(*mut libc::c_void, i32, *mut libc::c_void) -> *const libc::c_void;
//pub type FnGetClass = unsafe extern "C" fn(*const u8) -> *const libc::c_void;

pub fn acquire_anisette_v1() {
    unsafe {
        use winapi::um::libloaderapi::*;
        use std::mem::transmute;
        let flags = LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SAFE_CURRENT_DIRS | LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS;
        let sup_path = to_wide_chars("C:/Program Files (x86)/Common Files/Apple/Apple Application Support");
        let _ret = AddDllDirectory(sup_path.as_ptr());
        let sup_path = to_wide_chars("C:/Program Files (x86)/Common Files/Apple/Internet Services");
        let _ret = AddDllDirectory(sup_path.as_ptr());
        
        // Load libraries
        //DisableThreadLibraryCalls(hLibModule)
        let objc = LoadLibraryExA(
            "C:/Program Files (x86)/Common Files/Apple/Apple Application Support/objc.dll\0".as_ptr() as _,
            std::ptr::null_mut(),
            flags.clone()
        );
        let cf = LoadLibraryExA(
            "C:/Program Files (x86)/Common Files/Apple/Apple Application Support/CoreFoundation.dll\0".as_ptr() as _,
            std::ptr::null_mut(),
            flags.clone()
        );
        let cfnet = LoadLibraryExA(
            "C:/Program Files (x86)/Common Files/Apple/Apple Application Support/CFNetwork.dll\0".as_ptr() as _,
            std::ptr::null_mut(),
            flags.clone()
        );
        let icloud = LoadLibraryExA(
            "C:/Program Files (x86)/Common Files/Apple/Internet Services/iCloud_main.dll\0".as_ptr() as _,
            std::ptr::null_mut(),
            flags.clone()
        );
        // Get functions
        let get_class = transmute::<FARPROC, FnGetClass>(GetProcAddress(objc, "objc_getClass\0".as_ptr() as _));
        let sel_register_name = transmute::<FARPROC, FnSelRegisterName>(GetProcAddress(objc, "sel_registerName\0".as_ptr() as _));
        GetProcAddress(objc, "objc_msgSend\0".as_ptr() as _);
        GetProcAddress(objc, "class_copyMethodList\0".as_ptr() as _);
        GetProcAddress(objc, "method_getName\0".as_ptr() as _);
        GetProcAddress(objc, "sel_getName\0".as_ptr() as _);
        GetProcAddress(objc, "object_getClass\0".as_ptr() as _);

        let cf_release = transmute::<FARPROC, FnCFRelease>(GetProcAddress(cf, "CFRelease\0".as_ptr() as _));
        
        let addr = transmute::<FARPROC, usize>(GetProcAddress(icloud, "PL_FreeArenaPool\0".as_ptr() as _));
        let cpy_anisette: FnCopyAnisette = transmute(addr + 0x241ee0 - 0x1aa2a0);

        let _res = cpy_anisette(std::ptr::null_mut(), 1, std::ptr::null_mut());

        winapi::um::libloaderapi::FreeLibrary(icloud);
        winapi::um::libloaderapi::FreeLibrary(cf);
        winapi::um::libloaderapi::FreeLibrary(objc);
    }
}

thread_local! {
    pub static ATTR_TLS: u32 = 0;
}

pub fn acquire_anisette_v2() {
    use memmodpp::Memmod;
    let buf = Vec::new();
    let memmod = Memmod::load(&buf, "iCloud", 0);
}

#[cfg(test)] 
mod tests {
    use super::*;
    
    #[test]
    fn test_acquire_anisette() {
        acquire_anisette_v1();
        acquire_anisette_v2();
    }
}