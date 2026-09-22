// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/llvm/lowering/values.hpp"

#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/enumeration.hpp"
#include "tetrodotoxin/library/language/constants/flag.hpp"
#include "tetrodotoxin/library/language/constants/implementation.hpp"
#include "tetrodotoxin/library/language/constants/object.hpp"
#include "tetrodotoxin/library/language/constants/option.hpp"
#include "tetrodotoxin/library/language/constants/range.hpp"
#include "tetrodotoxin/library/language/constants/real.hpp"
#include "tetrodotoxin/library/language/constants/result.hpp"
#include "tetrodotoxin/library/language/constants/signed.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/terminal/llvm/lowering/types.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Terminal;
using namespace Tetrodotoxin::Library::Language;
using Llvm::Emission::States;

auto Llvm::Lowering::Values::lower(
    const Execution& execution,
    const Constant& expression) -> Bool {
  const States& body = execution.get_states();

  auto flag = expression.select<Constants::Flag>();
  if (flag) {
    return Types::prepare(execution.get_program(), flag->get_type()) &&
           body.unsigned_value(
               flag->get_type(), *flag, U64(bool(flag->get_value())));
  }

  auto unsigned_value = expression.select<Constants::Unsigned>();
  if (unsigned_value) {
    return Types::prepare(
               execution.get_program(), unsigned_value->get_type()) &&
           body.unsigned_value(
               unsigned_value->get_type(), *unsigned_value,
               unsigned_value->get_value());
  }

  auto signed_value = expression.select<Constants::Signed>();
  if (signed_value) {
    return Types::prepare(execution.get_program(), signed_value->get_type()) &&
           body.signed_value(
               signed_value->get_type(), *signed_value,
               signed_value->get_value());
  }

  auto real = expression.select<Constants::Real>();
  if (real) {
    return Types::prepare(execution.get_program(), real->get_type()) &&
           body.real_value(real->get_type(), *real, real->get_value());
  }

  auto bytes = expression.select<Constants::Bytes>();
  if (bytes) {
    return Types::prepare(execution.get_program(), bytes->get_type()) &&
           body.bytes_value(
               bytes->get_type(), *bytes, bytes->get_value(),
               bytes->get_resource());
  }

  auto object = expression.select<Constants::Object>();
  if (object) {
    return Types::prepare(execution.get_program(), object->get_type()) &&
           body.object_value(object->get_type(), *object);
  }

  auto implementation = expression.select<Constants::Implementation>();
  if (implementation) {
    return Types::prepare(
               execution.get_program(), implementation->get_type()) &&
           body.absent(implementation->get_type(), *implementation);
  }

  auto enumeration = expression.select<Constants::Enumeration>();
  if (enumeration) {
    return Types::prepare(execution.get_program(), enumeration->get_type()) &&
           body.unsigned_value(
               enumeration->get_type(), *enumeration, enumeration->get_value());
  }

  auto option = expression.select<Constants::Option>();
  if (option) {
    BAIL_IF(!Types::prepare(execution.get_program(), option->get_type()));
    if (option->get_kind() ==
        Tetrodotoxin::Library::Language::Types::Option::Kind::Absent) {
      return body.absent(option->get_type(), *option);
    }

    auto payload = option->get_payload();
    return payload && execution.lower(*payload) &&
           body.present(
               option->get_type(), option->get_type().get_element_type(),
               *option, *payload);
  }

  auto result = expression.select<Constants::Result>();
  if (result) {
    return Types::prepare(execution.get_program(), result->get_type()) &&
           execution.lower(result->get_payload()) &&
           body.result(result->get_type(), *result, result->get_payload());
  }

  auto range = expression.select<Constants::Range>();
  if (range) {
    return Types::prepare(execution.get_program(), range->get_type()) &&
           execution.get_storage().empty_range(range->get_type(), *range);
  }

  return False;
}
