pub mod bom;
mod rle;
mod compress;
mod car;
mod rendition;
mod asset_catalog;

pub fn dump(path: &str) {
    let exists = std::fs::File::open(path).is_ok();
    if exists {
        let ldr = car::Loader::new(path);
        ldr.dump();
    } else {
        println!("Intput car file \"{path}\" not exist");
    }
}

pub fn compile(_asset_dir: &str, _output_dir: &str) {
}