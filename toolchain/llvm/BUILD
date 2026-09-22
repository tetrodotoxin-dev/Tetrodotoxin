# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"Pinned LLVM development SDK facade for the LLVM Terminal."

load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(default_visibility = ["//visibility:private"])

# The LLVM Terminal is the only product owner that consumes this facade. Keeping
# the external repository behind one narrow target prevents SDK headers and
# compiler flags from becoming ambient inputs to semantic code.
alias(
    name = "sdk",
    actual = "@llvm_22_1_8//:sdk",
    visibility = ["//tetrodotoxin/terminal:__pkg__"],
)

alias(
    name = "lld",
    actual = "@llvm_22_1_8//:lld",
    visibility = ["//tetrodotoxin/terminal:__pkg__"],
)

cc_test(
    name = "static_lld_test",
    srcs = ["static_lld.cpp"],
    deps = [":lld"],
)

exports_files([
    "MANIFEST.md",
    "README.md",
])
