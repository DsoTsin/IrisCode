// packager -o [output.pak] [directory0] [directory1] ...
//extern crate compression;
use clap::Parser;

#[derive(Parser)]
#[command(name = "packager")]
#[command(version = "1.0")]
#[command(author, about, long_about = None)]
struct Cli {
    #[arg(short, long)]
    output: String,
    dirs: Vec<String>,
}

fn main() {
    let cli = Cli::parse();
    println!("output: {:?}, dirs: {:?}", &cli.output, &cli.dirs);
    ipak::package(&cli.dirs, &cli.output)
        .unwrap_or_default();
}