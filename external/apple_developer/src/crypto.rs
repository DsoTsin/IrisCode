#![allow(dead_code)]

#[repr(C)]
pub(crate) struct ccdigest_info {
    pub output_size: usize,
    pub state_size: usize,
    pub block_size: usize,
    pub oid_size: usize,
    pub oid: *mut u8,
    pub init_state: *const libc::c_void,

}

extern "C" {
    fn ccdigest_init(di: *mut ccdigest_info, ctx: *mut libc::c_void);
    fn ccdigest_update(di: *mut ccdigest_info, ctx: *mut libc::c_void, len: usize, data: *const u8);

    fn ccdigest(di: *const ccdigest_info, len: usize, data: *const u8, digest: *mut libc::c_void);

    //const struct ccdigest_info *ccsha256_di(void)
    fn ccsha256_di();

    //int ccpbkdf2_hmac(const struct ccdigest_info *di,
        // size_t passwordLen, const void *password,
        // size_t saltLen, const void *salt,
        // size_t iterations,
        // size_t dkLen, void *dk)
}