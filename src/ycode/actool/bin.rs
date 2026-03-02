use clap::{Parser, Args, Subcommand, ValueEnum};
// https://docs.rs/clap/latest/clap/_derive/index.html
#[derive(Parser, Debug)]
#[command(name = "actool")]
#[command(version = "1.0")]
#[command(author, about = "compiles, prints, updates, and verifies asset catalogs.", long_about = "actool verifies, updates, and prints the contents of an asset catalog, generating its output in standard plist format. The tool follows a \"read\", \"modify\", \"write\", \"print\" order of operations.")]
struct Cli {
    #[command(flatten)]
    option: Option<PackOptions>,
    #[command(subcommand)]
    command: Commands
}

#[derive(Subcommand, Debug)]
enum Commands {
    /// Compiles document and writes the output to the specified directory path. The name of the CAR file will be Assets.car. The compile option instructs actool to convert an asset catalog to files optimized for runtime. Additionally, --warnings, --errors, and --output-format are three other options that are commonly combined with --compile.
    #[command(name="--compile")]
    Compile { output_dir: String, input_asset_dir: String },
    /// Include a listing of the catalog's content in the output.
    #[command(name="--print-contents")]
    Dump { car_file: String },
}

#[derive(Args, Debug)]
struct PackOptions {
    /// Include document notice messages in actool's plist output. Notices will appear under the key com.apple.actool.document.notices, with messages listed under the subkey message and error types under the subkey type.
    #[arg(long)]
    notices: bool,
    /// Include document warning messages in actool's plist output. Warnings will appear under the key com.apple.actool.document.warnings, with messages listed under the subkey message and warning types under the subkey type.
    #[arg(long)]
    warnings: bool,
    /// Include document error messages in actool's plist output. Errors will appear under the key com.apple.actool.document.errors, with messages listed under the subkey message and error types under the subkey type.
    #[arg(long)]
    errors: bool,
    /// PNGs copied into iOS targets will be processed using pngcrush to optimize reading the images on iOS devices. This has no effect for images that wind up in the compiled CAR file, as it only affects PNG images copied in to the output bundle.
    #[arg(long)]
    compress_pngs: bool,
    /// By default, actool provides output in the form of an XML property list. Specifying 'binary1' will instruct actool to output a binary property list. Similarly, 'xml1' specifies an XML property list, and 'human-readable-text' specifies human readable text.
    #[arg(long)]
    output_format: Option<OutputFormat>,
    /// Emit a plist to path that contains keys and values to include in an application's info plist. path is the full path to the info plist, and should have the path extension .plist specified. The plist is populated with information gathered while compiling the CAR file, and currently contains information about the app icon and launch images used by the project. During builds, the information produced here will be merged into the target bundle's Info.plist.
    #[arg(long)]
    output_partial_info_plist: Option<String>,
    /// Can be combined with --compile to select a primary app icon. The app icon will either be copied into the output directory specified by --compile, or into the generated CAR file, depending on the value of --minimum-deployment-target. Deploying to macOS 10.13 or iOS 11.0 and later will cause the app icon to be included in the generated CAR file. A partially defined image is still generated into the output path, but this behavior may go away in the future. This flag also causes actool to declare the app icon in the partial info plist component specified by --output-partial-info-plist.
    #[arg(long)]
    app_icon: Option<String>,
    /// Can be combined with --compile to select a launch image to compile to the output directory, for most platforms. On tvOS, the launch image is compiled into the resulting CAR file. This flag also causes actool to declare the launch image in the partial info plist component specified by --output-partial-info-plist.
    #[arg(long)]
    launch_image: Option<String>,
    /// Sets the type of the product that's being built. In Xcode, all targets have a product type, and certain product types will cause slightly different behaviors in actool. These behaviors are currently centered around how stickers generate their content, as sticker packs have special requirements for where and how content should be formatter. actool currently recognizes two special product types: com.apple.product-type.app-extension.messages-sticker-pack and com.apple.product-type.app-extension.messages.
    #[arg(long)]
    product_type: Option<String>,
    /// Specifies the target platform to compile for. This option influences warnings, validation, and which images are included in the built product.
    #[arg(long)]
    platform: Option<String>,
    /// Causes actool to filter the files put into the CAR file by device. This simulates how the App Store will thin the developer's application. For example, if you pass iPhone9,1, actool will only include images appropriate to iPhone 7. This is useful for testing to make sure thinned applications will work properly. During build time, this is driven by the TARGET_DEVICE_MODEL build setting, and is selected by choosing the active run destination in the scheme pop-up. When the argument is not present, no thinning will occur.
    #[arg(long)]
    filter_for_device_model: Option<String>,
    /// Causes actool to filter the files put into the CAR file by OS version. This simulates how the App Store will thin the developer's application based on the final target OS of the app. For example, if you pass /fI11.0/fR, actool will only include images appropriate to iOS 11.0, but not previous versions. This is useful for testing to make sure thinned applications will work properly.
    #[arg(long)]
    filter_for_device_os_version: Option<String>,
    /// Specifies the target device to compile for, and may be passed multiple times. This option influences warnings, validation, and which images are included in the built product.
    #[arg(long)]
    target_device: Option<Vec<Device>>,
    /// Specifies the minimum deployment target to compile for. This option influences warnings, validation, and which images are included in the built product.
    #[arg(long)]
    minimum_deployment_target: Option<String>,
    /// Tells actool to process on-demand resources. This may result in multiple CAR files being produced. Without this option, actool ignores ODR tags found in the asset catalog.
    #[arg(long)]
    enable_on_demand_resources: Option<Bool>,
}

#[derive(ValueEnum, Debug, Clone)]
enum OutputFormat {
    #[value(name="xml1")]
    Xml,
    #[value(name="binary1")]
    Binary,
    #[value(name="human-readable-text")]
    Text
}

#[derive(ValueEnum, Debug, Clone)]
enum Device {
    #[value(name="iphone")]
    IPhone,
    #[value(name="ipad")]
    IPad,
    #[value(name="tv")]
    TV
}

#[derive(ValueEnum, Debug, Clone)]
enum Bool {
    #[value(name="YES")]
    Yes,
    #[value(name="NO")]
    No
}

fn main() {
    let args = Cli::parse();
    println!("{args:#?}");
    match &args.command {
        Commands::Compile { output_dir, input_asset_dir } => {
            asset_catalog::compile(input_asset_dir, output_dir);
        },
        Commands::Dump { car_file } => {
            asset_catalog::dump(car_file);
        },
    }
}