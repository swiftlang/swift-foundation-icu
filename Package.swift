// swift-tools-version: 5.7
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

var buildSettings: [CXXSetting] = [
    .define("DEBUG", to: "1", .when(configuration: .debug)),
    .define("U_ATTRIBUTE_DEPRECATED", to: ""),
    .define("U_SHOW_CPLUSPLUS_API", to: "1"),
    .define("U_SHOW_INTERNAL_API", to: "1"),
    .define("U_STATIC_IMPLEMENTATION"),
    .define("U_TIMEZONE", to: "_timezone", .when(platforms: [.windows])),
    .define("U_TIMEZONE", to: "timezone",
            .when(platforms: [
                .iOS,
                .macOS,
                .tvOS,
                .watchOS,
                .macCatalyst,
                .driverKit,
                .android,
                .linux,
            ])),
    .define("U_TIMEZONE_PACKAGE", to: "\"icutz44l\""),
    .define("U_HAVE_TZSET", to: "0", .when(platforms: [.wasi])),
    .define("U_HAVE_TZNAME", to: "0", .when(platforms: [.wasi])),
    .define("U_HAVE_TIMEZONE", to: "0", .when(platforms: [.wasi])),
    .define("FORTIFY_SOURCE", to: "2"),
    .define("STD_INSPIRED"),
    .define("MAC_OS_X_VERSION_MIN_REQUIRED", to: "101500"),
    .define("_WASI_EMULATED_SIGNAL", .when(platforms: [.wasi])),
    .define("_WASI_EMULATED_MMAN", .when(platforms: [.wasi])),
    .define("U_HAVE_STRTOD_L", to: "1"),
    .define("U_HAVE_XLOCALE_H", to: "1"),
    .define("U_HAVE_STRING_VIEW", to: "1"),
    .define("U_DISABLE_RENAMING", to: "1"),
    .define("U_COMMON_IMPLEMENTATION"),
    // Where data are stored
    .define("ICU_DATA_DIR", to: "\"/usr/share/icu/\""),
    .define("USE_PACKAGE_DATA", to: "1"),
    .define("APPLE_ICU_CHANGES", to: "1")
]

#if os(Windows)
buildSettings.append(contentsOf: [
    .define("_CRT_SECURE_NO_DEPRECATE"),
    .define("U_PLATFORM_USES_ONLY_WIN32_API"),
])
#endif

let commonBuildSettings: [CXXSetting] = buildSettings.appending([
    .headerSearchPath("."),
])

let i18nBuildSettings: [CXXSetting] = buildSettings.appending([
    .define("U_I18N_IMPLEMENTATION"),
    .define("SWIFT_PACKAGE", to: "1", .when(platforms: [.linux])),
    .headerSearchPath("../common"),
    .headerSearchPath("."),
])

let ioBuildSettings: [CXXSetting] = buildSettings.appending([
    .define("U_IO_IMPLEMENTATION"),
    .headerSearchPath("../common"),
    .headerSearchPath("../i18n"),
    .headerSearchPath("."),
])

let stubDataBuildSettings: [CXXSetting] = buildSettings.appending([
    .headerSearchPath("../common"),
    .headerSearchPath("../i18n"),
    .headerSearchPath("."),
])

let linkerSettings: [LinkerSetting] = [
    .linkedLibrary("wasi-emulated-signal", .when(platforms: [.wasi])),
    .linkedLibrary("wasi-emulated-mman", .when(platforms: [.wasi])),
]

let package = Package(
    name: "FoundationICU",
    products: [
        .library(
            name: "_FoundationICU",
            targets: ["_FoundationICU"]),
        .library(
            name: "_FoundationICUCommon",
            targets: ["_FoundationICUCommon"]),
        .library(
            name: "_FoundationICUI18N",
            targets: ["_FoundationICUI18N"]),
        .library(
            name: "_FoundationICUIO",
            targets: ["_FoundationICUIO"]),
    ],
    targets: [
        .target(
            name: "_FoundationICU",
            dependencies: [
                "_FoundationICUCommon",
                "_FoundationICUI18N",
                "_FoundationICUIO",
                "_FoundationICUStubData"
            ],
            path: "swift/FoundationICU"),
        .target(
            name: "_FoundationICUCommon",
            path: "icuSources/common",
            publicHeadersPath: "include",
            cxxSettings: commonBuildSettings),
        .target(
            name: "_FoundationICUI18N",
            dependencies: ["_FoundationICUCommon"],
            path: "icuSources/i18n",
            publicHeadersPath: "include",
            cxxSettings: i18nBuildSettings),
        .target(
            name: "_FoundationICUIO",
            dependencies: ["_FoundationICUCommon", "_FoundationICUI18N"],
            path: "icuSources/io",
            publicHeadersPath: "include",
            cxxSettings: ioBuildSettings),
        .target(
            name: "_FoundationICUStubData",
            dependencies: ["_FoundationICUCommon"],
            path: "icuSources/stubdata",
            publicHeadersPath: ".",
            cxxSettings: stubDataBuildSettings),
    ],
    cxxLanguageStandard: .cxx14
)

for target in package.targets {
    target.linkerSettings = linkerSettings
}

fileprivate extension Array {
    func appending(_ other: Self) -> Self {
        var me = self
        me.append(contentsOf: other)
        return me
    }
}
