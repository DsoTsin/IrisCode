# Built-in variables

* \_\_file\_\_ : str
* \_\_root_dir\_\_ : str
* \_\_build_dir\_\_ : str
* \_\_is_build\_\_ : bool
* \_\_is_generate\_\_: bool
* \_\_ide\_\_: str
* target_os : str, **currently supports:**
    * windows
    * linux
    * macos/ios
    * android
    * ps4/ps5
    * ohos, HUAWEI `HarmonyOS`
* target_arch : str, possible values:
    * x64/x86
    * arm64/arm
* target_config : str
    * debug/release/development(also for **profiling**)

## target built-in vars

* force_static_crt : bool, **build for windows**
* force_clang : bool, **build objc for windows**
* force_universal : bool, **build for macOS**
* platform_white_list : string_list
* configs : string_list
* output_dir : str
* precompiled_header : str
* precompiled_source : str
* visibility : string_list
* sources : string_list
* defines : string_list
* deps : string_list
* include_dirs : string_list
* lib_dirs : string_list
* libs : string_list
* ldflags : string_list
* `framework_dirs : string_list`, depend framework search directories.
* `frameworks : string_list`, depend frameworks.
* cflags : string_list
* clangs_cc/objc/**objcc**/ispc/swift/rust : string_list

## Target os built-in variables

* Windows
    * `msvc.sys_incs` : string_list
    * `msvc.cflags` : string_list
    * `msvc.cflags_cc` : string_list
    * `msvc.has_clang` : bool
    * `msvc.clang.root` : str
    * `msvc.clang.path` : str
    * `msvc.clang.ldflags` : string_list
    * `msvc.clang.cflags` : string_list
    * `msvc.clang.cflags_asm` : string_list
    * `msvc.clang.cflags_cc` : string_list
    * `msvc.clang.cflags_objc` : string_list
    * `msvc.clang.cflags_objcc` : string_list
* Playstation
    * Both: `has_ps4_support` : bool, `has_ps5_support` : bool.
    * `ps4.clang.root` : str
    * `ps4.clang.path` : str
    * `ps4.clang.cflags` : string_list
    * `ps4.clang.cflags_asm` : string_list
    * `ps4.clang.cflags_cc` : string_list
    * `ps4.clang.cflags_objc` : string_list
    * `ps4.clang.cflags_objcc` : string_list
    * `ps5.clang.root` : str
    * `ps5.clang.path` : str
    * `ps5.clang.cflags` : string_list
    * `ps5.clang.cflags_asm` : string_list
    * `ps5.clang.cflags_cc` : string_list
    * `ps5.clang.cflags_objc` : string_list
    * `ps5.clang.cflags_objcc` : string_list
    * `ps5.clang.ldflags` : string_list
* Android (WIP)
    * `android.java_home` : str
    * `android.sdk.home` : str
    * `android.ndk.home` : str
    * `android.clang.root` : str
    * `android.clang.path` : str
    * `android.clang.cflags` : string_list
    * `android.clang.cflags_asm` : string_list
    * `android.clang.cflags_cc` : string_list
    * `android.clang.cflags_objc` : string_list
    * `android.clang.cflags_objcc` : string_list
* iOS (WIP)
    * `ios.build_on_windows` : bool
    * `ios.clang.sysroot` : str
    * `ios.clang.cflags` : string_list
    * `ios.clang.cflags_cc` : string_list
    * `ios.clang.cflags_objc` : string_list
    * `ios.clang.cflags_objcc` : string_list
    * `ios.clang.cflags_asm` : string_list
* MacOS (WIP)
    * `macos.build_on_windows` : bool
    * `macos.clang.sysroot` : str
    * `macos.clang.cflags` : string_list
    * `macos.clang.cflags_cc` : string_list
    * `macos.clang.cflags_objc` : string_list
    * `macos.clang.cflags_objcc` : string_list
    * `macos.clang.cflags_asm` : string_list
* HarmonyOS (TODO)
* Linux (TODO)

## Language built-in variables

* ISPC
    * `has_ispc` : bool
    * `ispc.os` : str
* Swift
* Rust
* Java

---

### Example

``` gn
shared_library("newLibrary") {
  force_static_crt = true
  force_clang = false
  platform_white_list = ["windows"]
  output_dir = "$__build_dir__"
  sources = ["main.cc"]
  include_dirs = ["."]
  clangs_cc = ["/O0", "/g"]
  libs = ["user32.lib"]
}

```

---

# Scope Expression

variable scope

``` c
xen = {
    l = [5,3]
    p = "nig"
}
// xen.l : list
// xen.p : str
```

named scope

variable has serveral named scope

``` c
prog("xxx") {
    l = [5,3]
    p = "nig"
}
// prog.xxx.l : list
// prog.xxx.p : str
```

--- 

# Function Call Expression

```c
ret = execute("ls /") // without named arg
ret = execute(command = "ls /") // with named args
```

## Lambda function

```cpp
    download_and_extract(predownload_url, "../temp")
    execute("xbgn --build ../")
}
on_prebuild = () {
}
on_post_build = () {
    pack(target)
}
```

## Target Creation Functions

* `static_library`(name) { `scope` }
* `shared_library`(name) { `scope` }
* `framework`(name) { `scope` }
    * `resources` : string_list
    * `o_type` : str, `executable`/`bundle`/**`dynamic library`**(**default**)/`static library`
* `executable`(name) { `scope` }
    * `as_apple_app` : bool, macOS/iOS app bundle, if `target_os` is `ios`, it is `true`.
    * `pack_ios_ipa` : bool, **iOS** specific
    * `app_id` : str, used for both `android apk` and `apple app`
    * `apple_plist_info` : str
    * `apple_code_sign_id` : str, code signing entity for apple app
    * `as_android_apk` : bool, for android app target (pure `native`), otherwise android `elf` generated.
    * `as_ohos_hap` : bool, for Huawei's `OHOS` app 
* `android_apk`(name) { `scope` }
    * `compileSdkVersion`: int
    * `defaultConfig`: scope
        * `applicationId`: str
        * `minSdkVersion`: int
        * `targetSdkVersion`: int
        * `versionCode`: int
        * `versionName`: str
    * `java`: scope
        * `srcDirs`: list
        * `jarLibs`: list
    * `resDir`: str
    * `manifest`: str
    * `jnis`: list

### Rust cargo integration

```
confg ("rust") {
    registry = "https://rsproxy.cn/crates-io.index"
    toolchain = "nightly" // or stable, need to add required targets, rustup target add ....
}

// cargo build model
rust_cargo("package_name") {
    features = ["a", "b"] // optional
    version = "1.0" // optional
    path = "b/s" // local file path, optional
    git = "https://github.com/a/b" // optional
}

shared_library("test_lib") {
    sources = ["a.ispc", "b.rs", "c.cu", "d.hip", "ffi.cpp"]
    deps = ["package_name"]
}

rust_proc_macro("syn_rs") {
    path = "c/lib.rs"
    rust_deps = [ 
        "syn": {
            "version": "1.0"
        }, 
        "quote": {
            "version": "1.0"
        }
    ]
}

executable ("test") {
    sources = ["main.cc", "part1.rs", "ui.swift"]
    deps = ["package_name", "test_lib", "syn_rs"]
}
```

### Maven support (Java)

## Config

**config**: defines configuration for targets.

    The values specifiede directly on the target.

**variables** in a config definition

    Flags: cflags, cflags_cc, cflags_objc, cflags_asm, cflags_objcc, cflags_ispc, cflags_swift, cflags_rust
           defines, include_dirs, ldflags, lib_dirs, libs, precompiled_header, precompiled_source, frameworks, framework_dirs 

### Example

``` gn
config("core_config") {
    include_dirs = [ ".", "../build" ]
    defines = [ "DXC=1" ]
}

shared_library("cc") {
    sources = ...
    public_configs += [ ":core_config" ]
}

executable("test") {
    configs += [ ":core_config" ]
}
```

## Source set

    Flags: cflags, cflags_cc, cflags_objc, cflags_asm, cflags_objcc, cflags_ispc, cflags_swift, cflags_rust
           defines, include_dirs, ldflags, lib_dirs, libs, precompiled_header, precompiled_source, frameworks, framework_dirs 

    Deps: data_deps, deps, public_deps
    Dependent configs: configs, public_configs
    General: check_includes, configs, data, friend, inputs,
             public, sources, testonly,
             visibility


## Builtin Functions

* **zip**(file_list, root_dir, dest_file): compress file_list to single dest zip under root dir. 
* **unzip**(src_path, dst_dir): extract zip file from source path to dest dir.
* **execute**(cmd) -> (code, stdout, stderr): execute command line
* **download**(url, file, proxy): download file to file path
* **git_clone**(url, dir)
* `source_set`
* `config`
* `group`

``` c
    download(url = "http://sss.xx/ff.zip", file = "xx.zip")
    unzip("xx.zip", dest_dir)
```

---

# If And Match Expression

``` gn
if(var) {
    echo("hello var")
}

match(target_os) {
    "windows" | "ios" => {
        echo("Wow~~~")
    },
    "ps5" => {
        echo("Wow...")
    }
}
```

---

# Loop Expression (WIP)

``` gn
for i = 0...8 {

}

for i in tests 
{
   // continue
}

// need parse dict
for i in [
    "ss", //string 
    "uu" : { 
        "8788" : true,
        "sds" : 88 
        } // dict
    ] // array
{

  break
}
```