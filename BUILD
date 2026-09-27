# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@tetro_toolchain//:package.bzl", "package_release")

package(default_visibility = ["//visibility:public"])

exports_files(["LICENSE"])

alias(
    name = "build",
    actual = "//source/puffer:puffer",
)

package_release(
    name = "sdk",
    target = "//source/puffer:puffer",
)
