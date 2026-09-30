# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_import.bzl", "cc_import")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_import(
    name = "ffi",
    shared_library = "usr/lib/x86_64-linux-gnu/libffi.so.8",
)

cc_import(
    name = "client",
    shared_library = "usr/lib/x86_64-linux-gnu/libwayland-client.so.0",
    deps = [":ffi"],
)

# Only Linux links these native libraries. Other implementations do not include
# their headers or call their operations, and remain selected in the CPP files.
cc_library(
    name = "wayland",
    hdrs = glob(["usr/include/wayland-client*.h"]) + [
        "usr/include/wayland-util.h",
        "usr/include/wayland-version.h",
    ],
    includes = ["usr/include"],
    deps = select({
        "@platforms//os:linux": [":client"],
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
)

# Vendor notices stay with the acquired SDK alongside its original binaries.
exports_files([
    "usr/share/doc/libwayland-client0/copyright",
    "usr/share/doc/libffi8/copyright",
])
