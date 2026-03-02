/*
HMEMORYMODULE WINAPI LoadLibraryMemoryExA(
	_In_ PVOID BufferAddress,
	_In_ size_t Reserved,
	_In_opt_ LPCSTR DllBaseName,
	_In_opt_ LPCSTR DllFullName,
	_In_ DWORD Flags
);

HMEMORYMODULE WINAPI LoadLibraryMemoryExW(
	_In_ PVOID BufferAddress,
	_In_ size_t Reserved,
	_In_opt_ LPCWSTR DllBaseName,
	_In_opt_ LPCWSTR DllFullName,
	_In_ DWORD Flags
);

BOOL WINAPI FreeLibraryMemory(_In_ HMEMORYMODULE hMemoryModule);
 */

#[repr(transparent)]
pub struct Memmod(*mut usize);

impl Memmod {
    pub fn load(buf: &[u8], name: &str, flags: u32) -> Self {
        unsafe {
            let name = std::ffi::CString::new(name).unwrap();
            Self(LoadLibraryMemoryExA(buf.as_ptr(), buf.len(), name.as_ptr(), name.as_ptr(), flags))
        }
    }
}

impl Drop for Memmod {
    fn drop(&mut self) {
        unsafe {
            FreeLibraryMemory(self.0);
        }
    }
}


#[cfg(windows)]
extern "stdcall" {
    fn LoadLibraryMemoryExA(addr: *const u8, size: usize, base_name: *const i8, full_name: *const i8, flags: u32) -> *mut usize;
    fn LoadLibraryMemoryExW(addr: *const u8, size: usize, base_name: *const u16, full_name: *const u16, flags: u32) -> *mut usize;
    fn FreeLibraryMemory(module: *mut usize);
}