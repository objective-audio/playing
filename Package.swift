// swift-tools-version: 5.9
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "playing",
    platforms: [.macOS(.v10_15), .iOS(.v13), .macCatalyst(.v13)],
    products: [
        .library(
            name: "playing",
            targets: ["playing"]
        ),
    ],
    dependencies: [
        .package(url: "https://github.com/objective-audio/processing.git", branch: "master"),
    ],
    targets: [
        .target(
            name: "playing",
            dependencies: [
                .product(name: "processing", package: "processing")
            ],
            cSettings: [
                .unsafeFlags(["-fmodules"]),
            ]
        ),
        .testTarget(
            name: "playing-tests",
            dependencies: [
                "playing",
            ],
            cxxSettings: [
                .unsafeFlags(["-fcxx-modules"]),
            ]
        ),
    ],
    cLanguageStandard: .gnu18,
    cxxLanguageStandard: .gnucxx2b
)
