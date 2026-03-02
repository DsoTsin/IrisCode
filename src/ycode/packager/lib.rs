
use std::{
    collections::HashMap,
    io::Write
};
use walkdir::WalkDir;
use zstd::*;
use bincode::Options;

#[derive(serde::Serialize, serde::Deserialize, Debug)]
struct Header {
    magic: u32,
    version: u32,
    num_entries: u64,
    string_table_offset: u64,
    data_table_offset: u64,
}

#[derive(serde::Serialize, serde::Deserialize, Debug)]
struct Entry {
    n_offset: u64,
    n_size: u64,
    d_offset: u64,
    d_csize: u64,
    d_osize: u64,
}

pub fn package(input_dir: &[String], output_dir: &str) -> std::io::Result<()> {
    use std::{path::PathBuf, fs::{File, self}};
    use pathdiff::diff_paths;
    
    let path = PathBuf::from(output_dir);
    if !path.exists() {
        fs::create_dir_all(&path).unwrap();
    }
    let mut file = File::create(path.join("Asset.car")).unwrap();

    let mut map = HashMap::<String, (u64,Vec<u8>)>::new();
    for dir in input_dir {
        for entry in WalkDir::new(&dir) {
            if let Ok(fpath) = entry {
                if fpath.path().is_file() {
                    if let Some(rel) = diff_paths(fpath.path(), &dir) {
                        println!("Add {}", fpath.path().display());
                        let f = File::open(fpath.path())?;
                        let olen = f.metadata()?.len();
                        let data = encode_all(f, 4)?;
                        let mut name = rel.display().to_string();
                        name = name.replace("\\", "/");
                        map.insert(name, (olen, data));
                    }
                }
            }
        }
    }
    
    let mut string_table_offset = 0u64;
    let mut data_table_offset = 0u64;
    let mut entries = Vec::new();

    for (name, data) in map.iter() {
        let slen = name.len() as u64;
        let dlen = data.1.len() as u64;

        entries.push(Entry {
            n_offset: string_table_offset.clone(),
            n_size: slen.clone(),
            d_offset: data_table_offset.clone(),
            d_csize: dlen.clone(),
            d_osize: data.0,
        });

        string_table_offset += slen;
        data_table_offset += dlen;
    }

    let hdr = Header {
        magic: 98,
        version: 1,
        num_entries: entries.len() as _,
        string_table_offset: 0,
        data_table_offset: string_table_offset
    };

    bincode::DefaultOptions::new()
        .with_fixint_encoding()
        .with_little_endian()
        .serialize_into(&mut file, &hdr)
        .unwrap_or_default();

    for entry in entries {        
        bincode::DefaultOptions::new()
            .with_fixint_encoding()
            .with_little_endian()
            .serialize_into(&mut file, &entry)
            .unwrap_or_default();
    }

    for (name, _) in map.iter() {
        file.write_all(name.as_bytes())?;
    }

    for (_, data) in map.iter() {
        file.write_all(&data.1)?;
    }

    Ok(())
}

#[repr(C)]
pub struct Pak {
    entries: HashMap<String, Entry>
}

impl Pak {
    pub fn load(path: &str) -> Pak {
        use std::fs::File;
        let entries = HashMap::new();
        let mut file = File::open(path).unwrap();
        let hdr: Header = bincode::DefaultOptions::new()
            .with_fixint_encoding()
            .with_little_endian()
            .deserialize_from(&mut file).unwrap();
        if hdr.magic == 98 {
            // hdr.data_table_offset;
            // hdr.string_table_offset;
            for _i in 0..hdr.num_entries {
                let _ent: Entry = bincode::DefaultOptions::new()
                    .with_fixint_encoding()
                    .with_little_endian()
                    .deserialize_from(&mut file).unwrap();
                // ent.d_offset;
                // ent.d_csize;
                // ent.d_osize;

                // ent.n_offset;
                // ent.n_size;
                //file.seek(pos)
            }
        }
        Self { entries }
    }

    pub fn get_size(&self, _path: &str) -> Vec<u8> {
        vec![]
    }
}

impl Drop for Pak {
    fn drop(&mut self) {
        
    }
}

#[no_mangle]
pub extern "C" fn ipak_load(path: *const libc::c_char) -> *mut Pak {
    let path = unsafe { std::ffi::CStr::from_ptr(path) }.to_str().unwrap();
    Box::into_raw(Box::new(Pak::load(path)))
}

#[no_mangle]
pub extern "C" fn ipak_free(pak: *mut Pak) {
    let _ = unsafe { Box::from_raw(pak) };
}
