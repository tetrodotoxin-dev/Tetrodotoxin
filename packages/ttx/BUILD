# # Tetrodotoxin
# Copyright (c) 2023-present Matt Kaes and contributors

"""Canonical source Packages distributed with Tetrodotoxin."""

load("//toolchain:tetrodotoxin.bzl", "ttx_source")

package(default_visibility = ["//visibility:public"])

ttx_source(
    name = "perimortem_memory",
    src = "Perimortem.Memory/package.ttx",
)

ttx_source(
    name = "perimortem_math",
    src = "Perimortem.Math/package.ttx",
)

ttx_source(
    name = "perimortem_graphics",
    src = "Perimortem.Graphics/package.ttx",
    deps = [":perimortem_math"],
)

ttx_source(
    name = "perimortem_system",
    src = "Perimortem.System/package.ttx",
    deps = [
        ":perimortem_math",
        ":perimortem_memory",
    ],
)
