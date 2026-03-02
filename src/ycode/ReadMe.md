Ycode
===

A substitute for Xcode

include

- actool
- ibtool
- codesign
- ycrun
- ycode-select

---

## actool

    actool 
        [--output-format] human-readable-text 
        [--notices]
        [--warnings]
        --output-partial-info-plist ${PLIST} 
        --app-icon ${AppIcon}
        [--launch-image] ${LaunchImage}
        [--filter-for-device-model] ${MODEL}
        [--filter-for-device-os-version] ${Version}
        --product-type com.apple.product-type.application 
        [–target-device] iphone
        [–target-device] ipad
        --minimum-deployment-target [10.0|12.0] 
        --platform [iphoneos|appletvos]
        --enable-on-demand-resource YES 
        --compile ${OutDir} 
        ${InputDir}/Assets.xcassets

    actool --version
    actool --print-contents
    // Simplest usage
    actool --compile /tmp MyApp.xcassets

> Depends on libplist, zlib, lzfse

---

## ibtool

---

## codesign



> Depends on `adsapi`.dll

---

## ycrun

    ycrun --sdk <SDK name> --find <tool name>
    ycrun --sdk <SDK name> <tool name> <tool arguments>
    ycrun <tool name> <tool arguments>

## ycode-select

    ycode-select -h|--help
    ycode-select -s|--switch <path>
    ycode-select -p|--print-path
    ycode-select -v|--version

---

### Unreal Build Tool Validation

    UnrealBuildTool -Mode=ValidatePlatforms -allplatforms|-platform=IOS