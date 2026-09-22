# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""
Toolchain configuration for Perimortem

Currently supports:
- Arch Linux (Hyprland)

You'll need to make sure you install clang.

For debugging use the CodeLLDB extension in VSCode plus the included .vscode launch and task jsons.
"""

load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "ACTION_NAMES")
load(
    "@bazel_tools//tools/cpp:cc_toolchain_config_lib.bzl",
    "feature",
    "flag_group",
    "flag_set",
    "tool_path",
)
load("@rules_cc//cc/common:cc_common.bzl", "cc_common")
load("@rules_cc//cc/toolchains:cc_toolchain_config_info.bzl", "CcToolchainConfigInfo")

cpp_compile_actions = [
    ACTION_NAMES.cpp_compile,
]

c_compile_actions = [
    ACTION_NAMES.c_compile,
]

all_link_actions = [
    ACTION_NAMES.cpp_link_executable,
    # ACTION_NAMES.c_compile,
    ACTION_NAMES.cpp_link_nodeps_dynamic_library,
]

def _impl(ctx):
    tool_paths = [
        tool_path(
            # Compiler is referenced by the name "gcc" for historic reasons.
            name = "gcc",
            path = "/usr/bin/clang++",
        ),
        tool_path(
            # Compiler is referenced by the name "gcc" for historic reasons.
            name = "g++",
            path = "/usr/bin/clang++",
        ),
        tool_path(
            name = "ld",
            path = "/usr/bin/ldd",
        ),
        tool_path(
            name = "ar",
            path = "/usr/bin/ar",
        ),
        tool_path(
            name = "cpp",
            path = "/usr/bin/clang-cpp",
        ),
        tool_path(
            name = "gcov",
            path = "/bin/false",
        ),
        tool_path(
            name = "nm",
            path = "/usr/bin/nm",
        ),
        tool_path(
            name = "objdump",
            path = "/bin/false",
        ),
        tool_path(
            name = "strip",
            path = "/bin/false",
        ),
    ]

    features = [
        feature(
            name = "cpp_compiler_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = cpp_compile_actions,
                    flag_groups = ([
                        flag_group(
                            flags = [
                                # Always Wall and Werror.
                                # The goal is to bootstrap an 100% standard conforming codebase.
                                "-Wall",
                                "-Werror",
                                "-fno-exceptions",
                                "-fno-rtti",
                                "-mavx2",  # AVX2 support required
                                "-mrdrnd",  # _rdrand64_step
                                "-march=znver4",
                                "-std=c++26",
                                "-no-canonical-prefixes",
                            ],
                        ),
                    ]),
                ),
            ],
        ),
        feature(
            name = "c_compiler_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = c_compile_actions,
                    flag_groups = ([
                        flag_group(
                            flags = [
                                # Always Wall and Werror.
                                # The goal is to bootstrap an 100% standard conforming codebase.
                                "-Wall",
                                "-Werror",
                                "-Wno-character-conversion",  # google-test
                                "-fno-exceptions",
                                "-fno-rtti",
                                "-march=znver4",
                                "-std=c23",
                                "-no-canonical-prefixes",
                                # Force C builds from external libraries to build in C.
                                "-xc",
                            ],
                        ),
                    ]),
                ),
            ],
        ),
        feature(
            name = "default_linker_flags",
            enabled = True,
            flag_sets = [
                flag_set(
                    actions = all_link_actions,
                    flag_groups = ([
                        flag_group(
                            flags = [
                                "-lstdc++",
                            ],
                        ),
                    ]),
                ),
            ],
        ),
    ]

    return cc_common.create_cc_toolchain_config_info(
        ctx = ctx,
        features = features,
        cxx_builtin_include_directories = [
            "/usr/lib/clang/22/include",
            "/usr/lib/clang/21/include",
            "/usr/lib/clang/20/include",
            "/usr/include",
        ],
        toolchain_identifier = "k8-toolchain",
        host_system_name = "local",
        target_system_name = "local",
        target_cpu = "k8",
        target_libc = "unknown",
        compiler = "clang",
        abi_version = "unknown",
        abi_libc_version = "unknown",
        tool_paths = tool_paths,
    )

cc_toolchain_config = rule(
    implementation = _impl,
    attrs = {},
    provides = [CcToolchainConfigInfo],
)
