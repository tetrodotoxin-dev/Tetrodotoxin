// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/spirv/module/constants.hpp"

#include "perimortem/core/math.hpp"

#include "tetrodotoxin/library/language/constants/flag.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Terminal::Spirv;

auto Module::Constants::get_id(
    const Library::Language::Constant& constant) const -> Core::Option<U32> {
  for (const Entry& entry : entries.get_view()) {
    if (&entry.constant.get() == &constant) {
      return entry.id;
    }
  }
  return {};
}

auto Module::Constants::collect(const Library::Language::Constant& constant)
    -> Bool {
  if (get_id(constant)) {
    return True;
  }
  BAIL_IF(!types.collect(constant.get_type()));
  Bool supported = constant.is<Library::Language::Constants::Flag>() ||
                   constant.is<Library::Language::Constants::Real>() ||
                   constant.is<Library::Language::Constants::Signed>() ||
                   constant.is<Library::Language::Constants::Unsigned>();
  BAIL_IF(!supported);
  entries.insert(Entry(constant, ids.take()));
  return True;
}

auto Module::Constants::emit(Assembler::SpirV& assembler) const -> Bool {
  for (const Entry& entry : entries.get_view()) {
    const Library::Language::Constant& constant = entry.constant.get();
    auto type_id = types.get_id(constant.get_type());
    BAIL_IF(!type_id);

    auto flag = constant.select<Library::Language::Constants::Flag>();
    if (flag) {
      assembler.constant_flag(*type_id, entry.id, flag->get_value());
      continue;
    }
    auto real = constant.select<Library::Language::Constants::Real>();
    if (real) {
      auto type =
          real->get_type().select<Library::Language::Model::Types::Real>();
      BAIL_IF(!type);
      if (type->get_width() == 64) {
        assembler.constant_64(
            *type_id, entry.id, __builtin_bit_cast(U64, real->get_value()));
      } else {
        R32 narrowed = R32(real->get_value());
        assembler.constant(
            *type_id, entry.id, __builtin_bit_cast(U32, narrowed));
      }
      continue;
    }
    auto signed_value = constant.select<Library::Language::Constants::Signed>();
    if (signed_value) {
      S64 value = signed_value->get_value();
      BAIL_IF(!Core::Math::is_representable(value, sizeof(S32)));
      assembler.constant(*type_id, entry.id, U32(S32(value)));
      continue;
    }
    auto unsigned_value =
        constant.select<Library::Language::Constants::Unsigned>();
    BAIL_IF(!unsigned_value || unsigned_value->get_value() > U32(-1));
    assembler.constant(*type_id, entry.id, U32(unsigned_value->get_value()));
  }
  return True;
}
