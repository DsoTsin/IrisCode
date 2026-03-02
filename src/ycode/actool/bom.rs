#[allow(dead_code)]

type ResizeFn = Option<unsafe extern "C" fn (*mut BomContextMemory,usize)>;
type FreeFn = Option<unsafe extern "C" fn (*mut BomContextMemory)>;

#[repr(C)]
struct BomContextMemory {
    data: *mut libc::c_void,
    size: usize,
    resize: ResizeFn,
    free: FreeFn,
    ctx: *mut libc::c_void
}

#[repr(C)]
struct BomContext {
    _padding: u32
}

#[repr(C)]
struct BomTreeContext {
    _padding: u32
}

type IterateFn = Option<unsafe extern "C" fn (
    tree: *mut BomTreeContext,
    key: *const libc::c_void, klen: usize,
    value: *const libc::c_void, vlen: usize,
    context: *mut libc::c_void
)>;

unsafe extern "C" fn tree_iterate<F: Fn(&[u8],&[u8])>(
    _tree: *mut BomTreeContext,
    key: *const libc::c_void, klen: usize,
    value: *const libc::c_void, vlen: usize, 
    context: *mut libc::c_void
) {
    let ifn = context as *mut F;
    (*ifn)(
        std::slice::from_raw_parts(key as *const u8, klen),
        std::slice::from_raw_parts(value as *const u8, vlen)
    )
}

extern "C" {
    // fn bom_context_memory(data: *const libc::c_void, size: usize) -> BomContextMemory;
    // fn bom_context_memory_file(file_name: *const libc::c_char, writable: bool, min_size: usize) -> BomContextMemory;
    fn bom_read_file(file_name: *const libc::c_char) -> *mut BomContext;
    // fn bom_alloc_empty(mem: BomContextMemory) -> *mut BomContext;
    // fn bom_alloc_load(mem: BomContextMemory) -> *mut BomContext;
    fn bom_index_get(context: *mut BomContext, index: u32, data_len: *mut usize) -> *const u8;
    // fn bom_index_reserve(context: *mut BomContext, count: usize);
    // fn bom_index_add(context: *mut BomContext, data: *const u8, data_len: usize) -> u32;
    fn bom_variable_get(context: *mut BomContext, name: *const libc::c_char) -> i32;
    fn bom_variable_get_by_name(context: *mut BomContext, name: *const libc::c_char, data_len: *mut usize) -> *const u8;
    //fn bom_variable_add(context: *mut BomContext, name: *const libc::c_char, data_len: i32);
    fn bom_free(context: *mut BomContext);
    fn bom_tree_free(tree: *mut BomTreeContext);
    fn bom_tree_alloc_load(context: *mut BomContext, name: *const libc::c_char) -> *mut BomTreeContext;
    fn bom_tree_iterate(tree: *mut BomTreeContext, iterate: IterateFn, context: *const libc::c_void);
}

pub struct Context(*mut BomContext);

impl Context {
    pub fn load_file(path: &str) -> Self {
        let str = std::ffi::CString::new(path).unwrap();
        Self(unsafe { bom_read_file(str.as_ptr()) })
    }

    pub fn by_index(&self, index: usize) -> Option<&[u8]> {
        unsafe {
            let mut size = 0;
            let data = bom_index_get(self.0, index as u32, &mut size);
            if data.is_null() {
                None
            } else {
                Some(std::slice::from_raw_parts(data, size))
            }
        }
    }

    pub fn get<T>(&self, name: &str) -> &T {
        let bytes = self.by_name(name);
        let ptr = bytes.as_ptr() as *const T;
        unsafe { &*ptr }
    }

    pub fn by_name(&self, name: &str) -> &[u8] {
        unsafe {
            let str = std::ffi::CString::new(name).unwrap();
            let mut size = 0;
            let data = bom_variable_get_by_name(self.0, str.as_ptr(), &mut size);
            std::slice::from_raw_parts(data, size)
        }
    }

    pub fn get_index(&self, name: &str) -> usize {
        unsafe {
            let str = std::ffi::CString::new(name).unwrap();
            bom_variable_get(self.0, str.as_ptr()) as _
        }
    }

    pub fn iter<F: Fn(&[u8], &[u8])>(&self, name: &str, iterfn: F) {
        unsafe {
            let str = std::ffi::CString::new(name).unwrap();
            let tree = bom_tree_alloc_load(self.0, str.as_ptr());
            let pfn: *const F = &iterfn;
            if !tree.is_null() {
                bom_tree_iterate(tree, Some(tree_iterate::<F>), pfn as _);
                bom_tree_free(tree);
            }
        }
    }
}

impl Drop for Context {
    fn drop(&mut self) {
        unsafe { bom_free(self.0); }
    }
}