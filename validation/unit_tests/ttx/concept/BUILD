# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

cc_library(
    name = "subject_fixture",
    testonly = True,
    hdrs = ["fixtures/subject.hpp"],
    deps = ["//ttx:concept"],
)

cc_binary(
    name = "libabstract_subject.so",
    testonly = True,
    srcs = ["fixtures/subject.cpp"],
    linkshared = True,
    deps = [":subject_fixture"],
)

exports_files(
    ["sentinels.cpp"],
    visibility = ["//validation:__pkg__"],
)

# Core graph checks must remain usable without a Source dialect or a compiler.
cc_binary(
    name = "concept_tests",
    testonly = True,
    srcs = [
        "abstract.cpp",
        "fixtures/observation.c",
        "fixtures/observation.h",
        "sentinels.cpp",
        "modules/module.cpp",
        "//validation:unit_test.cpp",
        "//validation:unit_tests/process/child.cpp",
    ],
    data = [":libabstract_subject.so"],
    deps = [
        ":subject_fixture",
        "//ttx:concept",
        "//validation:process_fixture",
        "//validation:test_headers",
    ],
)
