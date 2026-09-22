// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Validation::Process::Fixture {

auto dispatch(S32 argument_count, const char* const arguments[], S32& status)
    -> Bool;

}  // namespace Validation::Process::Fixture
