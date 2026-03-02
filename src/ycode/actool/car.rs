#![allow(dead_code)]

use std::io::Write;

use super::rendition::{CSIHeader, CUIThemePixelRendition, CUIThemeTextureRendition};

use super::bom::Context;
//use super::rendition::*;

// @see https://github.com/joey-gm/Aphrodite/blob/master/Aphrodite/Models/Attributes.swift
// https://github.com/joey-gm/Aphrodite/blob/master/Aphrodite/Models/CoreUI_Reference.swift
// https://blog.timac.org/2018/1112-quicklook-plugin-to-visualize-car-files/
// _CSIRenditionBlockData expandCSIBitmapData:fromSlice:makeReadOnly
#[repr(C)]
pub struct Header {
    tag: [u8;4],
    core_ui_ver: u32,
    storage_ver: u32,
    storage_ts: u32,
    rendition_count: u32,
    major_ver_str: [u8;128],
    ver_str: [u8;256],
    uuid: [u8;16],
    checksum: u32,
    schema_ver: u32,
    color_space_id: u32,
    key_semantics: u32,
}

impl std::fmt::Debug for Header {
    fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
        use std::ffi::CStr;
        let tag = std::str::from_utf8(&self.tag).unwrap();
        let major_ver_str = unsafe { CStr::from_ptr(self.major_ver_str.as_ptr() as _) };
        let ver_str = unsafe { CStr::from_ptr(self.ver_str.as_ptr() as _) };
        f.debug_struct("Header")
            .field("tag", &tag)
            .field("CoreUI version", &self.core_ui_ver)
            .field("storage version", &self.storage_ver)
            .field("storage timestamp", &self.storage_ts)
            .field("rendition count", &self.rendition_count)
            .field("major version", &major_ver_str)
            .field("version", &ver_str)
            .field("uuid", &self.uuid)
            .field("color space id", &self.color_space_id)
            .finish()
    }
}

#[repr(C)]
pub struct ExtMetadata {
    tag: [u8;4],
    thinning_args: [u8; 256],
    platform_ver: [u8; 256],
    platform: [u8; 256],
    author_tool: [u8; 256]
}

impl std::fmt::Debug for ExtMetadata {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        use std::ffi::CStr;
        let tag = std::str::from_utf8(&self.tag).unwrap();
        let args = unsafe { CStr::from_ptr(self.thinning_args.as_ptr() as _) };
        let platform_ver = unsafe { CStr::from_ptr(self.platform_ver.as_ptr() as _) };
        let platform = unsafe { CStr::from_ptr(self.platform.as_ptr() as _) };
        let atool = unsafe { CStr::from_ptr(self.author_tool.as_ptr() as _) };
        f.debug_struct("ExtMetadata")
            .field("tag", &tag)
            .field("thinning args", &args)
            .field("platform version", &platform_ver)
            .field("platform", &platform)
            .field("author tool", &atool)
            .finish()
    }
}

pub struct Loader {
    bom: Context,
}

impl Loader {
    pub fn new(path: &str) -> Self {
        Self {
            bom: Context::load_file(path)
        }
    }

    pub fn dump(&self) {
        let header: &Header = self.bom.get("CARHEADER");
        println!("{:#?}", header);
        let metadata: &ExtMetadata = self.bom.get("EXTENDED_METADATA");
        println!("Meta {:#?}", metadata);
        self.bom.iter("FACETKEYS", |key, _value| {
            let key = std::str::from_utf8(&key).unwrap_or_default();
            println!("{:?}", &key);
        });
        
        self.bom.iter("APPEARANCEKEYS", |key, _value| {
            let key = std::str::from_utf8(&key).unwrap_or_default();
            println!("Appearance: {}", &key);
        });

        self.bom.iter("RENDITIONS", |key, value| {
            let csi = value.as_ptr() as *const CSIHeader;
            let csi = unsafe { &*csi };
            let offset  = std::mem::size_of::<CSIHeader>() + csi.tlv_length as usize + 4 + 4 as usize * csi.bitmaps.count as usize;
            let tag = &value[offset..offset+4];
            let rendition = &value[offset+4..];
            let tag = std::str::from_utf8(&tag).unwrap();
            match tag {
                "MLEC" | "CELM" => {
                    let px_rendition = rendition.as_ptr() as *const CUIThemePixelRendition;
                    println!("{:02X?} ->\n{:#?}\n{}:{}-{}\n{:#?}", &key, csi, &offset, value.len(), &tag, unsafe{&*px_rendition});
                }
                "RTXT" => {
                    let mut file = std::fs::File::create("target/rtxt.bin").unwrap();
                    file.write(&rendition).unwrap();
                    let tex_rendition = rendition.as_ptr() as *const CUIThemeTextureRendition;
                    println!("{:02X?} ->\n{:#?}\n{}:{}-{}\n{:#?}", &key, csi, &offset, value.len(), &tag, unsafe{&*tex_rendition});
                }
                a => {
                    println!("Unknown rendition tag '{}'", a);
                }
            }
        });
        let _kfmt = self.bom.by_name("KEYFORMAT");
    }
}

// Unity
// DataSet
// ImageSet
// ImageStack
// BrandAsssetGroup

// Normal App
// TextureSet / MipmapSet

// Unreal

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn car_test() {
        let ld = Loader::new("tests/compiled_assetcatelog/Assets.car");
        ld.dump();
    }
}