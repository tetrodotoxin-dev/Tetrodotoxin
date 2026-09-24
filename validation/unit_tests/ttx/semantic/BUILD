# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

# Measurement wraps process allocation and memmove. Keeping this package and
# executable separate prevents those hooks from entering general validation.
package(
    default_testonly = True,
    default_visibility = ["//visibility:private"],
)

cc_library(
    name = "provider_contract",
    hdrs = [
        "fixtures/heterogeneous_provider.h",
        "fixtures/provider.h",
        "fixtures/provider_representation.h",
        "fixtures/interface_provider.h",
        "fixtures/invocation_provider.h",
    ],
    deps = ["//ttx:data", "//ttx:semantic"],
)

cc_binary(
    name = "libflow_provider.so",
    srcs = ["fixtures/provider.c"],
    linkshared = True,
    copts = ["-std=c17", "-Werror", "-Wall", "-Wextra", "-pedantic-errors"],
    deps = [":provider_contract"],
)

cc_binary(
    name = "libheterogeneous_provider.so",
    srcs = ["fixtures/heterogeneous_provider.c"],
    linkshared = True,
    copts = ["-std=c17", "-Werror", "-Wall", "-Wextra", "-pedantic-errors"],
    deps = [":provider_contract"],
)

cc_binary(
    name = "libinterface_provider.so",
    srcs = ["fixtures/interface_provider.c"],
    linkshared = True,
    copts = ["-std=c17", "-Werror", "-Wall", "-Wextra", "-pedantic-errors"],
    deps = [":provider_contract"],
)

cc_binary(
    name = "libinvocation_provider.so",
    srcs = ["fixtures/invocation_provider.c"],
    linkshared = True,
    copts = ["-std=c17", "-Werror", "-Wall", "-Wextra", "-pedantic-errors"],
    deps = [":provider_contract"],
)

cc_binary(
    name = "flow_tests",
    srcs = [
        "transport/block.cpp",
        "negotiation/query.cpp",
        "transport/flow.cpp",
        "flows/compiled_copy.cpp",
        "flows/copy.cpp",
        "flows/heterogeneous.cpp",
        "flows/swizzle.cpp",
        "fixtures.cpp",
        "fixtures.hpp",
        "transport/fragment.cpp",
        "measurement.cpp",
        "measurement.hpp",
        "module.cpp",
        "module.hpp",
        "flows/operations/fragment.cpp",
        "transport/shared.cpp",
        "realization/simulacra.cpp",
        "realization/invocation.cpp",
        "//validation:unit_test.cpp",
        "//validation:unit_tests/process/child.cpp",
    ],
    data = [":libflow_provider.so", ":libheterogeneous_provider.so", ":libinterface_provider.so", ":libinvocation_provider.so"],
    linkopts = [
        "-ldl",
        "-Wl,--wrap=memmove",
        "-Wl,--wrap=__asan_memmove",
        "-Wl,--wrap=malloc",
        "-Wl,--wrap=calloc",
        "-Wl,--wrap=realloc",
        "-Wl,--wrap=aligned_alloc",
        "-Wl,--wrap=posix_memalign",
    ],
    deps = [
        ":provider_contract",
        "//validation/unit_tests/ttx/data:preparation",
        "//ttx:concept",
        "//ttx:data",
        "//ttx:semantic",
        "//validation:process_fixture",
        "//validation:test_headers",
    ],
)

# The same independent C descriptor publication is executable on the native
# host and through the browser toolchain. It requires no dlopen or test hooks.
cc_binary(
    name = "portable_binding",
    srcs = [
        "fixtures/portable_provider.c",
        "fixtures/portable_provider.h",
        "negotiation/portable.cpp",
    ],
    visibility = ["//visibility:public"],
    deps = ["//ttx:semantic"],
)
