// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/llvm/lowering/builtins.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/library/builtin/enum/name.hpp"
#include "tetrodotoxin/library/builtin/fixed/access.hpp"
#include "tetrodotoxin/library/builtin/fixed/view.hpp"
#include "tetrodotoxin/library/builtin/object/access.hpp"
#include "tetrodotoxin/library/builtin/object/capacity.hpp"
#include "tetrodotoxin/library/builtin/object/clone.hpp"
#include "tetrodotoxin/library/builtin/object/is_shared.hpp"
#include "tetrodotoxin/library/builtin/object/reserve.hpp"
#include "tetrodotoxin/library/builtin/object/view.hpp"
#include "tetrodotoxin/library/builtin/view/is_empty.hpp"
#include "tetrodotoxin/library/builtin/view/size.hpp"
#include "tetrodotoxin/library/builtin/view/slice.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/object_storage.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Terminal;
using namespace Tetrodotoxin::Library;
using Llvm::Emission::Invocation;

static auto select_type(const Tetrodotoxin::Source::Layout& layout, Count index)
    -> Core::Option<const Tetrodotoxin::Source::Type&> {
  auto entry = layout.get_abstract(index);
  return entry ? entry->resolve().select<Tetrodotoxin::Source::Type>()
               : Core::Option<const Tetrodotoxin::Source::Type&>();
}

static auto select_parameter(const Tetrodotoxin::Source::Callable& callable, Count index)
    -> Core::Option<const Tetrodotoxin::Source::Addressable&> {
  auto entry = callable.get_parameters().get_abstract(index);
  return entry ? entry->select<Tetrodotoxin::Source::Addressable>()
               : Core::Option<const Tetrodotoxin::Source::Addressable&>();
}

auto Llvm::Lowering::Builtins::lower(
    const Execution& execution,
    const Tetrodotoxin::Source::Callable& callable,
    const Library::Language::Model::Pack& result,
    Core::View::Vector<LLVMValueRef> inputs,
    Core::Option<const Library::Language::Model::Pack&> receiver_source)
    -> Core::Option<Bool> {
  const Invocation& body = execution.get_invocation();
  auto receiver = select_parameter(callable, 0);
  auto receiver_type =
      receiver ? receiver->get_type().select<Library::Language::Model::Type>()
               : Core::Option<const Library::Language::Model::Type&>();
  BAIL_IF(receiver && !receiver_type);
  auto result_type = select_type(callable.get_results(), 0);

  if (callable.is<Builtin::Fixed::View>() ||
      callable.is<Builtin::Fixed::Access>()) {
    BAIL_IF(
        !receiver || !result_type || !receiver_source ||
        inputs.get_size() != 1);
    auto bytes = receiver_source->select_identity<
        Tetrodotoxin::Library::Language::Constants::Bytes>();
    if (bytes && callable.is<Builtin::Fixed::View>()) {
      return execution.get_states().bytes_value(
          *result_type, result, bytes->get_value(), bytes->get_resource());
    }
    return body.borrow_fixed(
        result, *result_type, *receiver_type, *receiver_source, inputs[0]);
  }

  auto enum_name = callable.select<Builtin::Enum::Name>();
  if (enum_name) {
    auto enumeration =
        receiver_type
            ? receiver_type->select<
                  Tetrodotoxin::Library::Language::Types::Enumeration>()
            : Core::Option<
                  const Tetrodotoxin::Library::Language::Types::Enumeration&>();
    BAIL_IF(!enumeration || !result_type || inputs.get_size() != 1);
    Memory::Dynamic::Vector<U64> values;
    Memory::Dynamic::Vector<Core::View::Bytes> names;
    values.resize(enumeration->get_cases().get_size());
    names.resize(enumeration->get_cases().get_size());
    for (Count index = 0; index < enumeration->get_cases().get_size();
         index++) {
      auto value = enumeration->get_case_value(index);
      BAIL_IF(!value);
      values[index] = *value;
      names[index] = enumeration->get_case_name(index);
    }
    return execution.get_states().enumeration_name(
        result, *result_type, inputs[0], values.get_view(), names.get_view());
  }

  if (callable.is<Builtin::View::Size>()) {
    BAIL_IF(!receiver || !result_type || inputs.get_size() != 1);
    return body.get_size(result, *result_type, *receiver_type, inputs[0]);
  }
  if (callable.is<Builtin::View::IsEmpty>()) {
    BAIL_IF(!receiver || !result_type || inputs.get_size() != 1);
    return body.contiguous_is_empty(
        result, *result_type, *receiver_type, inputs[0]);
  }
  if (callable.is<Builtin::View::Slice>()) {
    BAIL_IF(!receiver || !result_type || inputs.get_size() != 3);
    return body.slice_view(
        result, *result_type, *receiver_type, inputs[0], inputs[1], inputs[2]);
  }

  if (callable.is<Builtin::Object::Capacity>()) {
    BAIL_IF(!receiver || !result_type || inputs.get_size() != 1);
    return body.object_capacity(
        result, *result_type, *receiver_type, inputs[0]);
  }
  if (callable.is<Builtin::Object::IsShared>()) {
    BAIL_IF(
        !receiver || !result_type || !receiver_source ||
        inputs.get_size() != 1);
    return body.object_is_shared(
        result, *result_type, *receiver_type, *receiver_source, inputs[0]);
  }
  if (callable.is<Builtin::Object::Clone>()) {
    BAIL_IF(!receiver || !receiver_source || inputs.get_size() != 1);
    return body.object_clone(
        result, *receiver_type, *receiver_source, inputs[0]);
  }
  if (callable.is<Builtin::Object::View>()) {
    BAIL_IF(!receiver || !result_type || inputs.get_size() != 1);
    return body.object_view(result, *result_type, *receiver_type, inputs[0]);
  }

  if (callable.is<Builtin::Object::Access>() ||
      callable.is<Builtin::Object::Reserve>()) {
    auto storage =
        receiver_type
            ? receiver_type->select<
                  Tetrodotoxin::Library::Language::Types::ObjectStorage>()
            : Core::Option<const Tetrodotoxin::Library::Language::Types::
                               ObjectStorage&>();
    auto fallback =
        storage ? storage->get_element_type().create_default(
                      execution.get_program().get_arena())
                : Core::Option<Tetrodotoxin::Library::Language::Model::Pack&>();
    BAIL_IF(
        !receiver || !result_type || !receiver_source || !fallback ||
        !execution.lower(*fallback));
    if (callable.is<Builtin::Object::Access>()) {
      return Bool(
          inputs.get_size() == 1 &&
          body.object_access(
              result, *result_type, *receiver_type, *receiver_source, inputs[0],
              *fallback));
    }

    return Bool(
        inputs.get_size() == 2 &&
        body.object_reserve(
            result, *result_type, *receiver_type, *receiver_source, inputs[0],
            inputs[1], *fallback));
  }

  return {};
}
