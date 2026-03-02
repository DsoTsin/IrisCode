// swift-tools-version: 6.1
// The swift-tools-version declares the minimum version of Swift required to build this package.

import Foundation
import PackageDescription

let package = Package(
    name: "IrisUI",

    platforms: [
        .macOS(.v14),
        .iOS(.v13),
        .tvOS(.v13),
    ],

    products: [
        .executable(name: "Example", targets: ["Example"]),
        .library(name: "IrisUI", targets: ["IrisUI"]),
    ],

    dependencies: [
        .package(
            url: "https://github.com/ShaftUI/swift-collections",
            .upToNextMinor(from: "1.3.0")
        ),
        .package(
            url: "https://github.com/ShaftUI/SwiftReload.git",
            branch: "main"
        ),
    ],

    targets: [
        .executableTarget(
            name: "Example",
            dependencies: [
                "IrisUI",
                .product(
                    name: "SwiftReload",
                    package: "SwiftReload",
                    condition: .when(platforms: [.linux, .macOS])
                ),
            ],
            swiftSettings: [
                .interoperabilityMode(.Cxx, .when(platforms: [.windows, .linux, .macOS])),
                .unsafeFlags(["-Xfrontend", "-enable-private-imports"]),
                .unsafeFlags(["-Xfrontend", "-enable-implicit-dynamic"]),
            ],
            linkerSettings: [
                .unsafeFlags(
                    ["-Xlinker", "--export-dynamic"],
                    .when(platforms: [.linux, .android])
                )
            ]
        ),
        
        .target(
            name: "IrisUI",
            dependencies: [
                .product(name: "Collections", package: "swift-collections"),
                "_IrisUI",
                "_Common"
            ],
            cxxSettings: [
                .headerSearchPath("../../external/std.ext/include"), // Default include path
                .headerSearchPath("../../src/common") // Additional include path
            ],
            swiftSettings: [
                .interoperabilityMode(.Cxx, .when(platforms: [.linux, .windows, .macOS]))
            ],
        ),

        .systemLibrary(
            name: "_IrisUI",
            path: "src/coreui",
            pkgConfig: nil,
            providers: nil
        ),

        .systemLibrary(
            name: "_Common",
            path: "src/common",
            pkgConfig: nil,
            providers: nil
        ),

        .testTarget(
            name: "IrisUITests",
            dependencies: [
                "IrisUI",
            ],
            swiftSettings: [
                .interoperabilityMode(.Cxx)
            ]
        ),

    ],
    cxxLanguageStandard: .cxx17
)
