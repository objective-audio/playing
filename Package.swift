// swift-tools-version: 5.9
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "audio-playing",
    platforms: [.macOS(.v14), .iOS(.v17), .macCatalyst(.v17)],
    products: [
        .library(
            name: "audio-playing",
            targets: ["audio-playing"]
        ),
    ],
    dependencies: [
        .package(url: "https://github.com/objective-audio/processing.git", branch: "master")
    ],
    targets: [
        .target(
            name: "audio-playing",
            dependencies: [
                .product(name: "audio-processing", package: "processing")
            ],
            cSettings: [
                .unsafeFlags(["-fmodules"]),
            ]
        ),
        .testTarget(
            name: "playing-tests",
            dependencies: [
                "audio-playing",
            ],
            cxxSettings: [
                .unsafeFlags(["-fcxx-modules"]),
            ]
        ),
    ],
    cLanguageStandard: .gnu18,
    cxxLanguageStandard: .gnucxx2b
)
