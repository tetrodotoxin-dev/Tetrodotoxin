load("@tetro_toolchain//:package.bzl", "source_files")

# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    strip_include_prefix = "/source",
    name = "model",
    deps = [
        ":execution",
        ":type",
    ],
)

# Plugins select model vocabularies independently of source and dialect code.
cc_library(
    strip_include_prefix = "/source",
    name = "type",
    srcs = glob(["type/**/*.cpp"]),
    hdrs = glob([
        "type/**/*.h",
        "type/**/*.hpp",
    ]),
    deps = [
        "@perimortem",
        "@ttx",
    ],
)

cc_library(
    strip_include_prefix = "/source",
    name = "execution",
    srcs = glob(["execution/**/*.cpp"]),
    hdrs = glob([
        "execution/**/*.h",
        "execution/**/*.hpp",
    ]),
    deps = [
        ":type",
        "@perimortem",
        "@ttx",
    ],
)

source_files()
