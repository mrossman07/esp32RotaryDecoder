// swift-tools-version: 5.10
import PackageDescription

let package = Package(
    name: "EncoderBLE",
    platforms: [.iOS(.v17), .macOS(.v14)],
    products: [
        .library(name: "EncoderBLE", targets: ["EncoderBLE"]),
    ],
    targets: [
        .target(name: "EncoderBLE"),
        .testTarget(name: "EncoderBLETests", dependencies: ["EncoderBLE"]),
    ]
)
