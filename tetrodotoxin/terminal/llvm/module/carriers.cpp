// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

// The native bridge enters LLVM before the Perimortem owner so LLVM's standard
// declarations remain confined to this implementation unit.
#if __has_include("llvm/IR/BasicBlock.h")
#include "llvm/IR/BasicBlock.h"
#else
#error LLVM BasicBlock is required by the Library native compiler
#endif

#include "perimortem/core/static/vector.hpp"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "perimortem/abi/core/object.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/language/model/types/flag.hpp"
#include "tetrodotoxin/library/language/model/types/real.hpp"
#include "tetrodotoxin/library/language/model/types/signed.hpp"
#include "tetrodotoxin/library/language/model/types/value.hpp"
#include "tetrodotoxin/library/language/types/access.hpp"
#include "tetrodotoxin/library/language/types/enumeration.hpp"
#include "tetrodotoxin/library/language/types/fixed.hpp"
#include "tetrodotoxin/library/language/types/implementation.hpp"
#include "tetrodotoxin/library/language/types/interface.hpp"
#include "tetrodotoxin/library/language/types/object.hpp"
#include "tetrodotoxin/library/language/types/object_storage.hpp"
#include "tetrodotoxin/library/language/types/option.hpp"
#include "tetrodotoxin/library/language/types/range.hpp"
#include "tetrodotoxin/library/language/types/result.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"
#include "tetrodotoxin/library/language/types/view.hpp"
#include "tetrodotoxin/terminal/abi/representation/name.hpp"
#include "tetrodotoxin/terminal/abi/symbol.hpp"
#include "tetrodotoxin/terminal/llvm/module/body.hpp"
#include "tetrodotoxin/terminal/llvm/module/carriers.hpp"
#include "tetrodotoxin/terminal/llvm/module/program.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Terminal;
using namespace Tetrodotoxin::Library;

static auto llvm_text(Core::View::Bytes value) -> llvm::StringRef {
  return llvm::StringRef(
      reinterpret_cast<const char*>(value.get_data()), value.get_size());
}

static auto get_target(Llvm::Module::Emission& program)
    -> Core::Option<Llvm::Module::Program&> {
  if (program.get_kind() == Llvm::Module::Emission::Kind::Module) {
    return static_cast<Llvm::Module::Program&>(program);
  }
  return static_cast<Llvm::Module::Body&>(program).get_program();
}

static auto get_body(Llvm::Module::Emission& body)
    -> Core::Option<Llvm::Module::Body&> {
  return body.get_kind() == Llvm::Module::Emission::Kind::Body
             ? Core::Option<Llvm::Module::Body&>(
                   static_cast<Llvm::Module::Body&>(body))
             : Core::Option<Llvm::Module::Body&>();
}

static auto get_program(Llvm::Module::Emission& body)
    -> Llvm::Module::Program& {
  auto selected = get_body(body);
  return selected ? selected->get_program()
                  : static_cast<Llvm::Module::Program&>(body);
}

static auto get_context(Llvm::Module::Program& program) -> llvm::LLVMContext& {
  return *llvm::unwrap(&program.get_context());
}

static auto get_module(Llvm::Module::Program& program) -> llvm::Module& {
  return *llvm::unwrap(&program.get_module());
}

static auto get_builder(Llvm::Module::Body& body) -> llvm::IRBuilder<>& {
  return *llvm::unwrap(body.get_builder());
}

static auto get_function(Llvm::Module::Body& body) -> llvm::Function& {
  return *llvm::unwrap<llvm::Function>(body.get_function());
}

static auto object_descriptor_name(
    Llvm::Module::Program& target,
    const Tetrodotoxin::Source::Type& type) -> Core::View::Bytes {
  auto structure = type.select<Language::Types::Structure>();
  if (target.get_unit().is_package_member() && structure &&
      structure->is_externally_reachable(*structure)) {
    Tetrodotoxin::Terminal::Abi::Symbol symbol(
        target.get_arena(), type,
        Tetrodotoxin::Terminal::Abi::Symbol::Kind::ObjectDescriptor,
        target.get_unit());
    return symbol.get_view();
  }
  Tetrodotoxin::Terminal::Abi::Representation::Name name(
      target.get_arena(), type,
      Tetrodotoxin::Terminal::Abi::Representation::Name::Kind::
          ObjectDescriptor);
  return name.get_view();
}

static auto object_descriptor_linkage(
    Llvm::Module::Program& target,
    const Tetrodotoxin::Source::Type& type) -> llvm::GlobalValue::LinkageTypes {
  auto structure = type.select<Language::Types::Structure>();
  return target.get_unit().is_package_member() && structure &&
                 structure->is_externally_reachable(*structure)
             ? llvm::GlobalValue::ExternalLinkage
             : llvm::GlobalValue::InternalLinkage;
}

static auto fail_toolchain(
    Llvm::Module::Emission& program,
    Core::View::Bytes message) -> Bool {
  auto target = get_target(get_program(program));
  return target ? target->fail_toolchain(message) : False;
}

static auto fail_type(
    Llvm::Module::Emission& program,
    const Tetrodotoxin::Source::Type& type,
    Core::View::Bytes message,
    Core::View::Bytes hint = {}) -> Bool {
  auto target = get_target(get_program(program));
  auto library_type =
      type.select<Tetrodotoxin::Library::Language::Model::Type>();
  auto anchor = library_type ? library_type->get_declaration_anchor()
                             : Core::Option<Tetrodotoxin::Source::Lexical::Anchor>();
  if (target && anchor) {
    return target->fail_source(*anchor, message, hint);
  }

  return fail_toolchain(program, message);
}

template <typename contract>
static auto select_contract(
    Llvm::Module::Emission& program,
    const Tetrodotoxin::Source::Type& type) -> Core::Option<const contract&> {
  auto selected = type.select<contract>();
  if (!selected) {
    fail_toolchain(
        program,
        "LLVM caller selected a carrier contract that does not match its source Type."_view);
  }

  return selected;
}

static auto select_field_type(const Tetrodotoxin::Source::Layout& fields, Count index)
    -> Core::Option<const Tetrodotoxin::Source::Type&> {
  auto entry = fields.get_abstract(index);
  auto field = entry ? entry->select<Tetrodotoxin::Source::Addressable>()
                     : Core::Option<const Tetrodotoxin::Source::Addressable&>();
  return field ? field->get_type().select<Tetrodotoxin::Source::Type>()
               : Core::Option<const Tetrodotoxin::Source::Type&>();
}

auto Llvm::Module::Carriers::publish(
    Llvm::Module::Emission& program,
    const Tetrodotoxin::Source::Type& type,
    Carrier carrier) const -> Core::Option<Bool> {
  auto found = carriers.find(&type);
  if (found) {
    if (found->value.kind != carrier.kind) {
      fail_toolchain(
          program,
          "LLVM received different carrier owners for one Type identity."_view);
      return {};
    }

    return False;
  }

  carriers.insert(&type, carrier);
  return True;
}

auto Llvm::Module::Carriers::reserve(
    Llvm::Module::Emission& program,
    const Tetrodotoxin::Source::Type& type,
    Kind kind) const -> Core::Option<Bool> {
  auto target = get_target(program);
  if (!target) {
    return {};
  }

  switch (kind) {
  case Kind::Value: {
    auto value =
        select_contract<Tetrodotoxin::Library::Language::Model::Types::Value>(
            program, type);
    if (!value) {
      return {};
    }

    Count width = value->get_width();
    Bool real =
        value->is<Tetrodotoxin::Library::Language::Model::Types::Real>();
    Bool signed_value =
        value->is<Tetrodotoxin::Library::Language::Model::Types::Signed>();
    Bool flag =
        value->is<Tetrodotoxin::Library::Language::Model::Types::Flag>();
    Core::Option<llvm::Type&> native;
    if (real && width == 32) {
      native = *llvm::Type::getFloatTy(get_context(*target));
    } else if (real && width == 64) {
      native = *llvm::Type::getDoubleTy(get_context(*target));
    } else if (!real && width != 0 && width <= U32(-1)) {
      native = *llvm::IntegerType::get(get_context(*target), U32(width));
    } else {
      fail_type(
          program, type,
          "LLVM cannot represent the selected scalar width."_view,
          "Select a nonzero integer width or a 32 or 64 bit Real Type."_view);
      return {};
    }

    Carrier carrier{
      .kind = kind,
      .native = llvm::wrap(&*native),
      .properties = U8(
          U8(real ? Carrier::Property::Real : Carrier::Property{}) |
          U8(signed_value ? Carrier::Property::Signed : Carrier::Property{}) |
          U8(flag ? Carrier::Property::Flag : Carrier::Property{})),
      .width = width,
    };
    return publish(program, type, carrier);
  }

  case Kind::Enumeration:
  case Kind::Fixed:
  case Kind::Range:
  case Kind::Context:
    return publish(program, type, Carrier{.kind = kind});

  case Kind::Option: {
    Tetrodotoxin::Terminal::Abi::Representation::Name name(
        target->get_arena(), type,
        Tetrodotoxin::Terminal::Abi::Representation::Name::Kind::OptionType);
    llvm::StructType& native = *llvm::StructType::create(
        get_context(*target), llvm_text(name.get_view()));
    return publish(
        program, type, Carrier{.kind = kind, .native = llvm::wrap(&native)});
  }

  case Kind::Result: {
    Tetrodotoxin::Terminal::Abi::Representation::Name name(
        target->get_arena(), type,
        Tetrodotoxin::Terminal::Abi::Representation::Name::Kind::ResultType);
    llvm::StructType& native = *llvm::StructType::create(
        get_context(*target), llvm_text(name.get_view()));
    return publish(
        program, type, Carrier{.kind = kind, .native = llvm::wrap(&native)});
  }

  case Kind::View:
  case Kind::Access: {
    Core::Static::Vector<llvm::Type*, 2> members = {{
      llvm::PointerType::getUnqual(get_context(*target)),
      llvm::Type::getInt64Ty(get_context(*target)),
    }};
    llvm::StructType& native = *llvm::StructType::get(
        get_context(*target),
        llvm::ArrayRef<llvm::Type*>(members.get_data(), members.get_size()));
    return publish(
        program, type, Carrier{.kind = kind, .native = llvm::wrap(&native)});
  }

  case Kind::Implementation: {
    Tetrodotoxin::Terminal::Abi::Representation::Name name(
        target->get_arena(), type,
        Tetrodotoxin::Terminal::Abi::Representation::Name::Kind::
            ImplementationType);
    Core::Static::Vector<llvm::Type*, 2> members = {{
      llvm::PointerType::getUnqual(get_context(*target)),
      llvm::PointerType::getUnqual(get_context(*target)),
    }};
    llvm::StructType& native = *llvm::StructType::create(
        get_context(*target), llvm_text(name.get_view()));
    native.setBody(
        llvm::ArrayRef<llvm::Type*>(members.get_data(), members.get_size()),
        false);
    return publish(
        program, type, Carrier{.kind = kind, .native = llvm::wrap(&native)});
  }

  case Kind::Structure: {
    Tetrodotoxin::Terminal::Abi::Representation::Name name(
        target->get_arena(), type,
        Tetrodotoxin::Terminal::Abi::Representation::Name::Kind::StructureType);
    llvm::StructType& native = *llvm::StructType::create(
        get_context(*target), llvm_text(name.get_view()));
    return publish(
        program, type,
        Carrier{
          .kind = kind,
          .native = llvm::wrap(&native),
          .payload = llvm::wrap(&native),
        });
  }

  case Kind::ObjectStorage: {
    llvm::Type& native = *llvm::PointerType::getUnqual(get_context(*target));
    return publish(
        program, type, Carrier{.kind = kind, .native = llvm::wrap(&native)});
  }

  case Kind::Object: {
    Tetrodotoxin::Terminal::Abi::Representation::Name payload_name(
        target->get_arena(), type,
        Tetrodotoxin::Terminal::Abi::Representation::Name::Kind::ObjectType);
    llvm::Type& native = *llvm::PointerType::getUnqual(get_context(*target));
    llvm::StructType& payload = *llvm::StructType::create(
        get_context(*target), llvm_text(payload_name.get_view()));
    return publish(
        program, type,
        Carrier{
          .kind = kind,
          .native = llvm::wrap(&native),
          .payload = llvm::wrap(&payload),
        });
  }
  }
}

auto Llvm::Module::Carriers::begin_completion(
    Llvm::Module::Emission& program,
    const Tetrodotoxin::Source::Type& type) const -> Core::Option<Bool> {
  auto found = carriers.find(&type);
  if (!found) {
    fail_toolchain(
        program,
        "LLVM cannot complete a Type before its owner reserves a carrier."_view);
    return {};
  }

  if (found->value.phase != Phase::Reserved) {
    return False;
  }

  found->value.phase = Phase::Completing;
  return True;
}

auto Llvm::Module::Carriers::select_completion(
    Llvm::Module::Emission& program,
    const Tetrodotoxin::Source::Type& type,
    Kind kind) const -> Core::Option<Carrier&> {
  auto found = carriers.find(&type);
  if (!found || found->value.kind != kind ||
      found->value.phase != Phase::Completing) {
    fail_toolchain(
        program,
        "LLVM received a carrier completion outside its reserved phase."_view);
    return {};
  }

  return found->value;
}

auto Llvm::Module::Carriers::complete(
    Llvm::Module::Emission& program,
    const Tetrodotoxin::Source::Type& type,
    Kind kind) const -> Bool {
  switch (kind) {
  case Kind::Value: {
    auto carrier = select_completion(program, type, kind);
    if (!carrier || !carrier->native) {
      return False;
    }

    carrier->phase = Phase::Complete;
    return True;
  }

  case Kind::Enumeration: {
    auto enumeration =
        select_contract<Tetrodotoxin::Library::Language::Types::Enumeration>(
            program, type);
    if (!enumeration) {
      return False;
    }

    auto storage = enumeration->get_storage_type();
    if (!storage) {
      return fail_type(
          program, type,
          "LLVM received the Enumeration carrier contract without its storage Type."_view,
          "Complete the exact Enumeration before lowering its carrier."_view);
    }

    auto carrier = select_completion(program, type, kind);
    auto storage_carrier = carriers.find(&*storage);
    auto native = get_type(*storage);
    if (!carrier || !storage_carrier ||
        storage_carrier->value.phase != Phase::Complete || !native) {
      return fail_toolchain(
          program,
          "LLVM cannot complete an Enumeration before its storage carrier."_view);
    }

    carrier->native = *native;
    carrier->element = *storage;
    carrier->set(Carrier::Property::Real, is_real(*storage));
    carrier->set(Carrier::Property::Signed, is_signed(*storage));
    carrier->phase = Phase::Complete;
    return True;
  }

  case Kind::Fixed: {
    auto fixed = select_contract<Tetrodotoxin::Library::Language::Types::Fixed>(
        program, type);
    if (!fixed) {
      return False;
    }

    const Tetrodotoxin::Source::Type& element = fixed->get_element_type();
    Count extent = Count(fixed->get_extent());
    auto carrier = select_completion(program, type, kind);
    auto element_carrier = carriers.find(&element);
    auto native = get_type(element);
    Bool element_ready = Bool(
        element_carrier &&
        (element_carrier->value.phase == Phase::Complete ||
         element_carrier->value.kind == Kind::Object ||
         element_carrier->value.kind == Kind::ObjectStorage ||
         element_carrier->value.kind == Kind::Implementation));
    if (!carrier || !element_ready || !native || extent == 0) {
      return fail_toolchain(
          program,
          "LLVM cannot complete Fixed before its element carrier and extent."_view);
    }

    carrier->native =
        llvm::wrap(llvm::ArrayType::get(llvm::unwrap(*native), extent));
    carrier->element = element;
    carrier->extent = extent;
    carrier->phase = Phase::Complete;
    return True;
  }

  case Kind::Option: {
    auto option =
        select_contract<Tetrodotoxin::Library::Language::Types::Option>(
            program, type);
    if (!option) {
      return False;
    }

    const Tetrodotoxin::Source::Type& element = option->get_element_type();
    const Tetrodotoxin::Source::Type& flag = option->get_flag_type();
    auto carrier = select_completion(program, type, kind);
    auto element_carrier = carriers.find(&element);
    auto flag_carrier = carriers.find(&flag);
    auto payload = get_type(element);
    auto selected = get_type(flag);
    if (element_carrier && element_carrier->value.phase == Phase::Completing &&
        element_carrier->value.kind != Kind::Object &&
        element_carrier->value.kind != Kind::ObjectStorage &&
        element_carrier->value.kind != Kind::Implementation) {
      return fail_type(
          program, element,
          "Library Type recursively contains itself through inline target storage."_view,
          "Break the value cycle with Object or another reference carrier."_view);
    }

    Bool element_ready = Bool(
        element_carrier &&
        (element_carrier->value.phase == Phase::Complete ||
         element_carrier->value.kind == Kind::Object ||
         element_carrier->value.kind == Kind::ObjectStorage ||
         element_carrier->value.kind == Kind::Implementation));
    if (!carrier || !carrier->native || !element_ready || !flag_carrier ||
        flag_carrier->value.phase != Phase::Complete || !payload || !selected) {
      return fail_toolchain(
          program,
          "LLVM cannot complete Option before its payload and flag carriers."_view);
    }

    if (element_carrier->value.kind == Kind::Object) {
      carrier->native = *payload;
    } else {
      auto& native =
          *llvm::cast<llvm::StructType>(llvm::unwrap(*carrier->native));
      native.setBody({llvm::unwrap(*payload), llvm::unwrap(*selected)}, false);
    }
    carrier->element = element;
    carrier->flag = flag;
    carrier->phase = Phase::Complete;
    return True;
  }

  case Kind::Result: {
    auto result =
        select_contract<Tetrodotoxin::Library::Language::Types::Result>(
            program, type);
    if (!result) {
      return False;
    }

    const Tetrodotoxin::Source::Type& value = result->get_value_type();
    const Tetrodotoxin::Source::Type& error = result->get_error_type();
    const Tetrodotoxin::Source::Type& flag = result->get_flag_type();
    auto carrier = select_completion(program, type, kind);
    auto value_carrier = carriers.find(&value);
    auto error_carrier = carriers.find(&error);
    auto flag_carrier = carriers.find(&flag);
    auto native_value = get_type(value);
    auto native_error = get_type(error);
    auto native_flag = get_type(flag);
    Bool recursive_value = Bool(
        value_carrier && value_carrier->value.phase == Phase::Completing &&
        value_carrier->value.kind != Kind::Object &&
        value_carrier->value.kind != Kind::ObjectStorage &&
        value_carrier->value.kind != Kind::Implementation);
    Bool recursive_error = Bool(
        error_carrier && error_carrier->value.phase == Phase::Completing &&
        error_carrier->value.kind != Kind::Object &&
        error_carrier->value.kind != Kind::ObjectStorage &&
        error_carrier->value.kind != Kind::Implementation);
    if (recursive_value || recursive_error) {
      return fail_type(
          program, type,
          "Library Result recursively contains itself through inline target storage."_view,
          "Break the value cycle with Object or another reference carrier."_view);
    }

    Bool value_ready = Bool(
        value_carrier && (value_carrier->value.phase == Phase::Complete ||
                          value_carrier->value.kind == Kind::Object ||
                          value_carrier->value.kind == Kind::ObjectStorage ||
                          value_carrier->value.kind == Kind::Implementation));
    Bool error_ready = Bool(
        error_carrier && (error_carrier->value.phase == Phase::Complete ||
                          error_carrier->value.kind == Kind::Object ||
                          error_carrier->value.kind == Kind::ObjectStorage ||
                          error_carrier->value.kind == Kind::Implementation));
    if (!carrier || !carrier->native || !value_ready || !error_ready ||
        !flag_carrier || flag_carrier->value.phase != Phase::Complete ||
        !native_value || !native_error || !native_flag) {
      return fail_toolchain(
          program,
          "LLVM cannot complete Result before both alternatives and its flag carrier."_view);
    }

    auto target = get_target(program);
    if (!target) {
      return False;
    }

    const llvm::DataLayout& layout = get_module(*target).getDataLayout();
    llvm::Type* value_type = llvm::unwrap(*native_value);
    llvm::Type* error_type = llvm::unwrap(*native_error);
    Count value_size = layout.getTypeAllocSize(value_type).getFixedValue();
    Count error_size = layout.getTypeAllocSize(error_type).getFixedValue();
    Count value_alignment = layout.getABITypeAlign(value_type).value();
    Count error_alignment = layout.getABITypeAlign(error_type).value();
    llvm::Type* alignment_type =
        value_alignment >= error_alignment ? value_type : error_type;
    Count alignment_size =
        layout.getTypeAllocSize(alignment_type).getFixedValue();
    Count union_size = Core::Math::max(value_size, error_size);
    Count padding =
        union_size > alignment_size ? union_size - alignment_size : 0;
    llvm::SmallVector<llvm::Type*, 2> storage_members = {alignment_type};
    if (padding != 0) {
      storage_members.push_back(
          llvm::ArrayType::get(
              llvm::Type::getInt8Ty(get_context(*target)), padding));
    }

    llvm::StructType& storage =
        *llvm::StructType::get(get_context(*target), storage_members, false);
    auto& native =
        *llvm::cast<llvm::StructType>(llvm::unwrap(*carrier->native));
    native.setBody({&storage, llvm::unwrap(*native_flag)}, false);
    carrier->payload = llvm::wrap(&storage);
    carrier->element = value;
    carrier->error = error;
    carrier->flag = flag;
    carrier->phase = Phase::Complete;
    return True;
  }

  case Kind::Range: {
    auto range = select_contract<Tetrodotoxin::Library::Language::Types::Range>(
        program, type);
    if (!range) {
      return False;
    }

    const Tetrodotoxin::Source::Type& element = range->get_element_type();
    auto target = get_target(program);
    auto carrier = select_completion(program, type, kind);
    auto element_carrier = carriers.find(&element);
    auto native = get_type(element);
    if (!target || !carrier || !element_carrier ||
        element_carrier->value.phase != Phase::Complete || !native) {
      return fail_toolchain(
          program,
          "LLVM cannot complete Range before its element carrier."_view);
    }

    carrier->native = llvm::wrap(
        llvm::StructType::get(
            get_context(*target),
            {llvm::unwrap(*native), llvm::unwrap(*native)}));
    carrier->element = element;
    carrier->phase = Phase::Complete;
    return True;
  }

  case Kind::View: {
    auto view = select_contract<Tetrodotoxin::Library::Language::Types::View>(
        program, type);
    if (!view) {
      return False;
    }

    return complete_contiguous(program, type, view->get_element_type(), kind);
  }

  case Kind::Access: {
    auto access =
        select_contract<Tetrodotoxin::Library::Language::Types::Access>(
            program, type);
    if (!access) {
      return False;
    }

    return complete_contiguous(program, type, access->get_element_type(), kind);
  }

  case Kind::Implementation: {
    auto implementation =
        select_contract<Tetrodotoxin::Library::Language::Types::Implementation>(
            program, type);
    auto carrier = select_completion(program, type, kind);
    if (!implementation || !carrier || !carrier->native) {
      return False;
    }

    auto interface =
        implementation->get_requirement()
            .resolve()
            .select<Tetrodotoxin::Library::Language::Types::Interface>();
    if (interface) {
      auto target = get_target(program);
      if (!target) {
        return False;
      }

      llvm::StructType& payload =
          *llvm::StructType::create(get_context(*target));
      carrier->payload = llvm::wrap(&payload);
      return complete_aggregate(
          program, type, interface->get_state_layout(), kind);
    }

    carrier->phase = Phase::Complete;
    return True;
  }

  case Kind::Structure: {
    auto structure =
        select_contract<Tetrodotoxin::Library::Language::Types::Structure>(
            program, type);
    if (!structure) {
      return False;
    }

    return complete_aggregate(program, type, structure->get_layout(), kind);
  }

  case Kind::Object: {
    auto object =
        select_contract<Tetrodotoxin::Library::Language::Types::Object>(
            program, type);
    if (!object) {
      return False;
    }

    return complete_aggregate(program, type, object->get_layout(), kind);
  }

  case Kind::ObjectStorage: {
    auto object =
        select_contract<Tetrodotoxin::Library::Language::Types::ObjectStorage>(
            program, type);
    if (!object) {
      return False;
    }

    auto carrier = select_completion(program, type, kind);
    const Tetrodotoxin::Source::Type& element = object->get_element_type();
    auto element_carrier = carriers.find(&element);
    if (!carrier || !element_carrier ||
        element_carrier->value.phase != Phase::Complete ||
        owns_resources(element)) {
      return fail_type(
          program, element,
          "LLVM Object[T] currently requires one value-only element Type."_view,
          "Use a scalar or Structure whose fields do not own Objects."_view);
    }

    carrier->element = element;
    carrier->phase = Phase::Complete;
    return True;
  }

  case Kind::Context: {
    auto carrier = select_completion(program, type, kind);
    if (!carrier) {
      return False;
    }

    carrier->phase = Phase::Complete;
    return True;
  }
  }
}

auto Llvm::Module::Carriers::complete_contiguous(
    Llvm::Module::Emission& program,
    const Tetrodotoxin::Source::Type& type,
    const Tetrodotoxin::Source::Type& element,
    Kind kind) const -> Bool {
  auto carrier = select_completion(program, type, kind);
  auto native = get_type(element);
  if (!carrier || !carrier->native || !native) {
    return fail_toolchain(
        program,
        "LLVM cannot complete contiguous storage before its element carrier."_view);
  }

  carrier->element = element;
  carrier->phase = Phase::Complete;
  return True;
}

auto Llvm::Module::Carriers::get_implementation_projection(
    Llvm::Module::Emission& program,
    const Tetrodotoxin::Source::Type& candidate) const -> Core::Option<LLVMValueRef> {
  auto target = get_target(program);
  auto object =
      candidate.select<Tetrodotoxin::Library::Language::Types::Object>();
  if (!target || !object) {
    return {};
  }

  Tetrodotoxin::Terminal::Abi::Symbol symbol(
      target->get_arena(), candidate,
      Tetrodotoxin::Terminal::Abi::Symbol::Kind::Projection,
      target->get_unit());
  llvm::Module& module = get_module(*target);
  if (llvm::GlobalVariable* existing =
          module.getNamedGlobal(llvm_text(symbol.get_view()))) {
    return llvm::wrap(existing);
  }

  Core::Static::Vector<llvm::Type*, 3> members = {{
    llvm::PointerType::getUnqual(get_context(*target)),
    llvm::Type::getInt64Ty(get_context(*target)),
    llvm::Type::getInt64Ty(get_context(*target)),
  }};
  llvm::StructType& projection_type = *llvm::StructType::get(
      get_context(*target),
      llvm::ArrayRef<llvm::Type*>(members.get_data(), members.get_size()));
  auto* projection = new llvm::GlobalVariable(
      module, &projection_type, true, llvm::GlobalValue::ExternalLinkage,
      nullptr, llvm_text(symbol.get_view()));
  return llvm::wrap(projection);
}

auto Llvm::Module::Carriers::complete_aggregate(
    Llvm::Module::Emission& program,
    const Tetrodotoxin::Source::Type& type,
    const Tetrodotoxin::Source::Layout& fields,
    Kind kind) const -> Bool {
  auto carrier = select_completion(program, type, kind);
  if (!carrier || !carrier->payload) {
    return False;
  }

  Memory::Dynamic::Vector<LLVMTypeRef> native_fields(fields.get_size());
  for (Count index = 0; index < fields.get_size(); index++) {
    auto entry = fields.get_abstract(index);
    auto field = entry ? entry->select<Tetrodotoxin::Source::Addressable>()
                       : Core::Option<const Tetrodotoxin::Source::Addressable&>();
    if (!field) {
      return fail_toolchain(
          program,
          "LLVM received an aggregate Layout entry without an Addressable."_view);
    }

    auto field_type = field->get_type().select<Tetrodotoxin::Source::Type>();
    if (!field_type) {
      return fail_toolchain(
          program,
          "LLVM cannot complete an aggregate before every Field Type."_view);
    }
    auto field_carrier = carriers.find(&*field_type);
    if (!field_carrier || !field_carrier->value.native) {
      return fail_toolchain(
          program,
          "LLVM cannot complete an aggregate before every Field carrier."_view);
    }

    Bool recursive_inline = Bool(
        field_carrier->value.phase == Phase::Completing &&
        field_carrier->value.kind != Kind::Object &&
        field_carrier->value.kind != Kind::ObjectStorage &&
        field_carrier->value.kind != Kind::Implementation &&
        kind != Kind::Object);
    if (recursive_inline) {
      return fail_type(
          program, type,
          "LLVM cannot lay out an inline Type that recursively contains itself."_view,
          "Break recursive inline storage with an Object reference or remove the recursive Field."_view);
    }

    native_fields.insert(*field_carrier->value.native);
    field_indices.insert(field.operator->(), index);
    field_hosts.insert(field.operator->(), &type);
  }

  llvm::cast<llvm::StructType>(llvm::unwrap(*carrier->payload))
      ->setBody(
          llvm::ArrayRef<llvm::Type*>(
              llvm::unwrap(native_fields.get_data()), native_fields.get_size()),
          false);
  carrier->fields = fields;
  carrier->phase = Phase::Complete;
  return True;
}

auto Llvm::Module::Carriers::get_type(const Tetrodotoxin::Source::Type& type) const
    -> Core::Option<LLVMTypeRef> {
  auto found = carriers.find(&type);
  return found && found->value.native ? found->value.native
                                      : Core::Option<LLVMTypeRef>();
}

auto Llvm::Module::Carriers::get_payload(const Tetrodotoxin::Source::Type& type) const
    -> Core::Option<LLVMTypeRef> {
  auto found = carriers.find(&type);
  return found && found->value.payload ? found->value.payload
                                       : Core::Option<LLVMTypeRef>();
}

auto Llvm::Module::Carriers::get_kind(const Tetrodotoxin::Source::Type& type) const
    -> Core::Option<Kind> {
  auto found = carriers.find(&type);
  if (!found || found->value.phase != Phase::Complete) {
    return {};
  }

  return found->value.kind;
}

auto Llvm::Module::Carriers::get_width(const Tetrodotoxin::Source::Type& type) const
    -> Core::Option<Count> {
  auto found = carriers.find(&type);
  if (!found || found->value.phase != Phase::Complete ||
      found->value.kind != Kind::Value || found->value.width == 0) {
    return {};
  }

  return found->value.width;
}

auto Llvm::Module::Carriers::get_element(const Tetrodotoxin::Source::Type& type) const
    -> Core::Option<const Tetrodotoxin::Source::Type&> {
  auto found = carriers.find(&type);
  if (!found || found->value.phase != Phase::Complete ||
      !found->value.element) {
    return {};
  }

  return *found->value.element;
}

auto Llvm::Module::Carriers::get_flag(const Tetrodotoxin::Source::Type& type) const
    -> Core::Option<const Tetrodotoxin::Source::Type&> {
  auto found = carriers.find(&type);
  if (!found || found->value.phase != Phase::Complete || !found->value.flag) {
    return {};
  }

  return *found->value.flag;
}

auto Llvm::Module::Carriers::get_error(const Tetrodotoxin::Source::Type& type) const
    -> Core::Option<const Tetrodotoxin::Source::Type&> {
  auto found = carriers.find(&type);
  if (!found || found->value.phase != Phase::Complete || !found->value.error) {
    return {};
  }

  return *found->value.error;
}

auto Llvm::Module::Carriers::get_extent(const Tetrodotoxin::Source::Type& type) const
    -> Core::Option<Count> {
  auto found = carriers.find(&type);
  if (!found || found->value.phase != Phase::Complete ||
      found->value.kind != Kind::Fixed || found->value.extent == 0) {
    return {};
  }

  return found->value.extent;
}

auto Llvm::Module::Carriers::get_fields(const Tetrodotoxin::Source::Type& type) const
    -> Core::Option<const Tetrodotoxin::Source::Layout&> {
  auto found = carriers.find(&type);
  if (!found || found->value.phase != Phase::Complete || !found->value.fields) {
    return {};
  }

  return *found->value.fields;
}

auto Llvm::Module::Carriers::get_field_index(
    const Tetrodotoxin::Source::Addressable& field) const -> Core::Option<Count> {
  auto found = field_indices.find(&field);
  return found ? Core::Option<Count>(found->value) : Core::Option<Count>();
}

auto Llvm::Module::Carriers::get_field_host(
    const Tetrodotoxin::Source::Addressable& field) const
    -> Core::Option<const Tetrodotoxin::Source::Type&> {
  auto found = field_hosts.find(&field);
  return found ? Core::Option<const Tetrodotoxin::Source::Type&>(*found->value)
               : Core::Option<const Tetrodotoxin::Source::Type&>();
}

auto Llvm::Module::Carriers::is_real(const Tetrodotoxin::Source::Type& type) const
    -> Bool {
  auto found = carriers.find(&type);
  return found ? found->value.has(Carrier::Property::Real) : False;
}

auto Llvm::Module::Carriers::is_signed(const Tetrodotoxin::Source::Type& type) const
    -> Bool {
  auto found = carriers.find(&type);
  return found ? found->value.has(Carrier::Property::Signed) : False;
}

auto Llvm::Module::Carriers::is_flag(const Tetrodotoxin::Source::Type& type) const
    -> Bool {
  auto found = carriers.find(&type);
  return found ? found->value.has(Carrier::Property::Flag) : False;
}

auto Llvm::Module::Carriers::is_object(const Tetrodotoxin::Source::Type& type) const
    -> Bool {
  auto found = carriers.find(&type);
  return Bool(found && found->value.kind == Kind::Object);
}

auto Llvm::Module::Carriers::zero(
    Llvm::Module::Emission& program,
    const Tetrodotoxin::Source::Type& type) const -> Core::Option<LLVMValueRef> {
  auto native = get_type(type);
  if (!native) {
    fail_toolchain(
        program,
        "LLVM cannot create a zero value without a completed carrier."_view);
    return {};
  }

  return llvm::wrap(llvm::Constant::getNullValue(llvm::unwrap(*native)));
}

auto Llvm::Module::Carriers::owns_resources(const Tetrodotoxin::Source::Type& type) const
    -> Bool {
  Memory::Dynamic::Vector<Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Type>>
      active;
  return owns_resources(type, active);
}

auto Llvm::Module::Carriers::owns_resources(
    const Tetrodotoxin::Source::Type& type,
    Memory::Dynamic::Vector<Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Type>>&
        active) const -> Bool {
  auto found = carriers.find(&type);
  if (!found) {
    return False;
  }

  const Carrier& carrier = found->value;
  if (carrier.kind == Kind::Object || carrier.kind == Kind::ObjectStorage ||
      carrier.kind == Kind::Implementation) {
    return True;
  }

  Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Type> retained(type);
  if (active.contains(retained)) {
    return False;
  }

  active.insert(retained);
  Bool result = False;
  if ((carrier.kind == Kind::Option || carrier.kind == Kind::Fixed) &&
      carrier.element) {
    result = owns_resources(*carrier.element, active);
  } else if (carrier.kind == Kind::Result && carrier.element && carrier.error) {
    result = owns_resources(*carrier.element, active) ||
             owns_resources(*carrier.error, active);
  } else if (carrier.kind == Kind::Structure && carrier.fields) {
    for (Count index = 0; index < carrier.fields->get_size(); index++) {
      auto field = select_field_type(*carrier.fields, index);
      if (field && owns_resources(*field, active)) {
        result = True;
        break;
      }
    }
  }

  active.remove(active.get_size() - 1);
  return result;
}

auto Llvm::Module::Carriers::retain(
    Llvm::Module::Emission& body,
    const Tetrodotoxin::Source::Type& type,
    LLVMValueRef value) const -> Bool {
  auto native_body = get_body(body);
  auto found = carriers.find(&type);
  if (!native_body || !found || !value) {
    return fail_toolchain(
        get_program(body),
        "LLVM cannot retain a value without its completed carrier."_view);
  }

  const Carrier& carrier = found->value;
  llvm::IRBuilder<>& builder = get_builder(*native_body);
  llvm::Value& native_value = *llvm::unwrap(value);
  if (!owns_resources(type)) {
    return True;
  }

  if (carrier.kind == Kind::Object || carrier.kind == Kind::ObjectStorage) {
    if (llvm::isa<llvm::ConstantPointerNull>(&native_value)) {
      return True;
    }

    auto target = get_target(get_program(body));
    if (!target) {
      return False;
    }

    llvm::FunctionType& signature = *llvm::FunctionType::get(
        llvm::Type::getVoidTy(get_context(*target)),
        {llvm::PointerType::getUnqual(get_context(*target))}, false);
    builder.CreateCall(
        get_module(*target).getOrInsertFunction(
            llvm_text(Perimortem::Abi::Core::object_retain_symbol), &signature),
        {&native_value});
  } else if (carrier.kind == Kind::Implementation) {
    llvm::Value& object = *builder.CreateExtractValue(&native_value, U32(0));
    if (llvm::isa<llvm::ConstantPointerNull>(&object)) {
      return True;
    }

    auto target = get_target(get_program(body));
    if (!target) {
      return False;
    }
    llvm::FunctionType& signature = *llvm::FunctionType::get(
        llvm::Type::getVoidTy(get_context(*target)),
        {llvm::PointerType::getUnqual(get_context(*target))}, false);
    builder.CreateCall(
        get_module(*target).getOrInsertFunction(
            llvm_text(Perimortem::Abi::Core::object_retain_symbol), &signature),
        {&object});
  } else if (carrier.kind == Kind::Option && carrier.element) {
    if (is_object(*carrier.element)) {
      return retain(body, *carrier.element, value);
    }

    llvm::Value& selected = *builder.CreateExtractValue(&native_value, U32(1));
    auto constant = llvm::dyn_cast<llvm::ConstantInt>(&selected);
    if (constant) {
      if (constant->isZero()) {
        return True;
      }

      llvm::Value& payload = *builder.CreateExtractValue(&native_value, U32(0));
      return retain(body, *carrier.element, llvm::wrap(&payload));
    }

    llvm::BasicBlock& copy = *llvm::BasicBlock::Create(
        get_function(*native_body).getContext(), "option.copy",
        &get_function(*native_body));
    llvm::BasicBlock& done = *llvm::BasicBlock::Create(
        get_function(*native_body).getContext(), "option.copy.done",
        &get_function(*native_body));
    builder.CreateCondBr(&selected, &copy, &done);
    builder.SetInsertPoint(&copy);
    llvm::Value& payload = *builder.CreateExtractValue(&native_value, U32(0));
    if (!retain(body, *carrier.element, llvm::wrap(&payload))) {
      return False;
    }

    builder.CreateBr(&done);
    builder.SetInsertPoint(&done);
  } else if (carrier.kind == Kind::Result && carrier.element && carrier.error) {
    llvm::Value& selected = *builder.CreateExtractValue(&native_value, U32(1));
    auto constant = llvm::dyn_cast<llvm::ConstantInt>(&selected);
    if (constant) {
      Bool value_selected = !constant->isZero();
      auto selected_result = select_result(body, type, value, value_selected);
      const Tetrodotoxin::Source::Type& selected_type =
          value_selected ? *carrier.element : *carrier.error;
      return selected_result && retain(body, selected_type, *selected_result);
    }

    llvm::BasicBlock& value_block = *llvm::BasicBlock::Create(
        get_function(*native_body).getContext(), "result.copy.value",
        &get_function(*native_body));
    llvm::BasicBlock& error_block = *llvm::BasicBlock::Create(
        get_function(*native_body).getContext(), "result.copy.error",
        &get_function(*native_body));
    llvm::BasicBlock& done = *llvm::BasicBlock::Create(
        get_function(*native_body).getContext(), "result.copy.done",
        &get_function(*native_body));
    builder.CreateCondBr(&selected, &value_block, &error_block);

    builder.SetInsertPoint(&value_block);
    auto selected_value = select_result(body, type, value, True);
    if (!selected_value || !retain(body, *carrier.element, *selected_value)) {
      return False;
    }
    builder.CreateBr(&done);

    builder.SetInsertPoint(&error_block);
    auto selected_error = select_result(body, type, value, False);
    if (!selected_error || !retain(body, *carrier.error, *selected_error)) {
      return False;
    }
    builder.CreateBr(&done);
    builder.SetInsertPoint(&done);
  } else if (carrier.kind == Kind::Fixed && carrier.element) {
    for (Count index = 0; index < carrier.extent; index++) {
      llvm::Value& element =
          *builder.CreateExtractValue(&native_value, U32(index));
      if (!retain(body, *carrier.element, llvm::wrap(&element))) {
        return False;
      }
    }
  } else if (carrier.kind == Kind::Structure) {
    if (!carrier.fields) {
      return fail_toolchain(
          get_program(body),
          "LLVM cannot retain a Structure without its completed Layout."_view);
    }

    for (Count index = 0; index < carrier.fields->get_size(); index++) {
      auto field = select_field_type(*carrier.fields, index);
      if (!field) {
        return fail_toolchain(
            get_program(body),
            "LLVM cannot retain a Structure with an invalid Field edge."_view);
      }

      if (!owns_resources(*field)) {
        continue;
      }

      llvm::Value& selected =
          *builder.CreateExtractValue(&native_value, U32(index));
      if (!retain(body, *field, llvm::wrap(&selected))) {
        return False;
      }
    }
  }

  return True;
}

auto Llvm::Module::Carriers::release(
    Llvm::Module::Emission& body,
    const Tetrodotoxin::Source::Type& type,
    LLVMValueRef value) const -> Bool {
  auto native_body = get_body(body);
  auto found = carriers.find(&type);
  if (!native_body || !found || !value) {
    return fail_toolchain(
        get_program(body),
        "LLVM cannot release a value without its completed carrier."_view);
  }

  const Carrier& carrier = found->value;
  llvm::IRBuilder<>& builder = get_builder(*native_body);
  llvm::Value& native_value = *llvm::unwrap(value);
  if (!owns_resources(type)) {
    return True;
  }

  if (carrier.kind == Kind::Object || carrier.kind == Kind::ObjectStorage) {
    if (llvm::isa<llvm::ConstantPointerNull>(&native_value)) {
      return True;
    }

    auto target = get_target(get_program(body));
    if (!target) {
      return False;
    }

    llvm::FunctionType& signature = *llvm::FunctionType::get(
        llvm::Type::getVoidTy(get_context(*target)),
        {llvm::PointerType::getUnqual(get_context(*target))}, false);
    builder.CreateCall(
        get_module(*target).getOrInsertFunction(
            llvm_text(Perimortem::Abi::Core::object_release_symbol),
            &signature),
        {&native_value});
  } else if (carrier.kind == Kind::Implementation) {
    llvm::Value& object = *builder.CreateExtractValue(&native_value, U32(0));
    if (llvm::isa<llvm::ConstantPointerNull>(&object)) {
      return True;
    }

    auto target = get_target(get_program(body));
    if (!target) {
      return False;
    }
    llvm::FunctionType& signature = *llvm::FunctionType::get(
        llvm::Type::getVoidTy(get_context(*target)),
        {llvm::PointerType::getUnqual(get_context(*target))}, false);
    builder.CreateCall(
        get_module(*target).getOrInsertFunction(
            llvm_text(Perimortem::Abi::Core::object_release_symbol),
            &signature),
        {&object});
  } else if (carrier.kind == Kind::Option && carrier.element) {
    if (is_object(*carrier.element)) {
      return release(body, *carrier.element, value);
    }

    llvm::Value& selected = *builder.CreateExtractValue(&native_value, U32(1));
    auto constant = llvm::dyn_cast<llvm::ConstantInt>(&selected);
    if (constant) {
      if (constant->isZero()) {
        return True;
      }

      llvm::Value& payload = *builder.CreateExtractValue(&native_value, U32(0));
      return release(body, *carrier.element, llvm::wrap(&payload));
    }

    llvm::BasicBlock& drop = *llvm::BasicBlock::Create(
        get_function(*native_body).getContext(), "option.drop",
        &get_function(*native_body));
    llvm::BasicBlock& done = *llvm::BasicBlock::Create(
        get_function(*native_body).getContext(), "option.drop.done",
        &get_function(*native_body));
    builder.CreateCondBr(&selected, &drop, &done);
    builder.SetInsertPoint(&drop);
    llvm::Value& payload = *builder.CreateExtractValue(&native_value, U32(0));
    if (!release(body, *carrier.element, llvm::wrap(&payload))) {
      return False;
    }

    builder.CreateBr(&done);
    builder.SetInsertPoint(&done);
  } else if (carrier.kind == Kind::Result && carrier.element && carrier.error) {
    llvm::Value& selected = *builder.CreateExtractValue(&native_value, U32(1));
    auto constant = llvm::dyn_cast<llvm::ConstantInt>(&selected);
    if (constant) {
      Bool value_selected = !constant->isZero();
      auto selected_result = select_result(body, type, value, value_selected);
      const Tetrodotoxin::Source::Type& selected_type =
          value_selected ? *carrier.element : *carrier.error;
      return selected_result && release(body, selected_type, *selected_result);
    }

    llvm::BasicBlock& value_block = *llvm::BasicBlock::Create(
        get_function(*native_body).getContext(), "result.drop.value",
        &get_function(*native_body));
    llvm::BasicBlock& error_block = *llvm::BasicBlock::Create(
        get_function(*native_body).getContext(), "result.drop.error",
        &get_function(*native_body));
    llvm::BasicBlock& done = *llvm::BasicBlock::Create(
        get_function(*native_body).getContext(), "result.drop.done",
        &get_function(*native_body));
    builder.CreateCondBr(&selected, &value_block, &error_block);

    builder.SetInsertPoint(&value_block);
    auto selected_value = select_result(body, type, value, True);
    if (!selected_value || !release(body, *carrier.element, *selected_value)) {
      return False;
    }
    builder.CreateBr(&done);

    builder.SetInsertPoint(&error_block);
    auto selected_error = select_result(body, type, value, False);
    if (!selected_error || !release(body, *carrier.error, *selected_error)) {
      return False;
    }
    builder.CreateBr(&done);
    builder.SetInsertPoint(&done);
  } else if (carrier.kind == Kind::Fixed && carrier.element) {
    for (Count index = carrier.extent; index != 0; index--) {
      llvm::Value& element =
          *builder.CreateExtractValue(&native_value, U32(index - 1));
      if (!release(body, *carrier.element, llvm::wrap(&element))) {
        return False;
      }
    }
  } else if (carrier.kind == Kind::Structure) {
    if (!carrier.fields) {
      return fail_toolchain(
          get_program(body),
          "LLVM cannot release a Structure without its completed Layout."_view);
    }

    for (Count index = carrier.fields->get_size(); index != 0; index--) {
      auto field = select_field_type(*carrier.fields, index - 1);
      if (!field) {
        return fail_toolchain(
            get_program(body),
            "LLVM cannot release a Structure with an invalid Field edge."_view);
      }

      if (!owns_resources(*field)) {
        continue;
      }

      llvm::Value& selected =
          *builder.CreateExtractValue(&native_value, U32(index - 1));
      if (!release(body, *field, llvm::wrap(&selected))) {
        return False;
      }
    }
  }

  return True;
}

auto Llvm::Module::Carriers::select_result(
    Llvm::Module::Emission& body,
    const Tetrodotoxin::Source::Type& type,
    LLVMValueRef value,
    Bool value_selected) const -> Core::Option<LLVMValueRef> {
  auto native_body = get_body(body);
  auto found = carriers.find(&type);
  auto native = get_type(type);
  if (!native_body || !found || !native || !value ||
      found->value.kind != Kind::Result || !found->value.element ||
      !found->value.error ||
      llvm::unwrap(value)->getType() != llvm::unwrap(*native)) {
    fail_toolchain(
        get_program(body),
        "LLVM cannot select an alternative from an incomplete Result carrier."_view);
    return {};
  }

  const Tetrodotoxin::Source::Type& alternative =
      value_selected ? *found->value.element : *found->value.error;
  auto native_alternative = get_type(alternative);
  if (!native_alternative) {
    return {};
  }

  llvm::IRBuilder<>& builder = get_builder(*native_body);
  LLVMValueRef address = native_body->create_entry_alloca(
      *native, value_selected ? "result.value"_view : "result.error"_view);
  builder.CreateStore(llvm::unwrap(value), llvm::unwrap(address));
  llvm::Value* storage = builder.CreateStructGEP(
      llvm::unwrap(*native), llvm::unwrap(address), U32(0));
  return llvm::wrap(
      builder.CreateLoad(llvm::unwrap(*native_alternative), storage));
}

auto Llvm::Module::Carriers::assemble_result(
    Llvm::Module::Emission& body,
    const Tetrodotoxin::Source::Type& type,
    const Tetrodotoxin::Source::Type& alternative,
    Bool value_selected,
    Core::View::Vector<LLVMValueRef> elements) const
    -> Core::Option<LLVMValueRef> {
  auto native_body = get_body(body);
  auto found = carriers.find(&type);
  auto native = get_type(type);
  if (!native_body || !found || !native || found->value.kind != Kind::Result ||
      !found->value.element || !found->value.error) {
    return {};
  }

  const Tetrodotoxin::Source::Type& expected =
      value_selected ? *found->value.element : *found->value.error;
  if (&expected != &alternative) {
    return {};
  }

  auto payload = assemble(body, alternative, elements);
  if (!payload || !native_body->acquire(alternative, *payload)) {
    return {};
  }

  llvm::IRBuilder<>& builder = get_builder(*native_body);
  llvm::Type* native_type = llvm::unwrap(*native);
  llvm::Value* aggregate = llvm::UndefValue::get(native_type);
  aggregate = builder.CreateInsertValue(
      aggregate, value_selected ? builder.getTrue() : builder.getFalse(),
      U32(1));
  LLVMValueRef address = native_body->create_entry_alloca(
      *native, value_selected ? "result.value"_view : "result.error"_view);
  builder.CreateStore(aggregate, llvm::unwrap(address));
  llvm::Value* storage =
      builder.CreateStructGEP(native_type, llvm::unwrap(address), U32(0));
  builder.CreateStore(llvm::unwrap(*payload), storage);
  llvm::Value* selected =
      builder.CreateLoad(native_type, llvm::unwrap(address));
  native_body->mark_owned(type, llvm::wrap(selected));
  return llvm::wrap(selected);
}

auto Llvm::Module::Carriers::assemble(
    Llvm::Module::Emission& body,
    const Tetrodotoxin::Source::Type& type,
    Core::View::Vector<LLVMValueRef> elements) const
    -> Core::Option<LLVMValueRef> {
  auto native_body = get_body(body);
  auto found = carriers.find(&type);
  auto native = get_type(type);
  if (!native_body || !found || !native) {
    fail_toolchain(
        get_program(body),
        "LLVM cannot assemble a value without its completed carrier."_view);
    return {};
  }

  for (LLVMValueRef element : elements) {
    if (!element) {
      fail_toolchain(
          get_program(body),
          "LLVM received an absent native value during assembly."_view);
      return {};
    }
  }

  llvm::Type& native_type = *llvm::unwrap(*native);
  if (elements.get_size() == 1 &&
      llvm::unwrap(elements[0])->getType() == &native_type) {
    return elements[0];
  }

  const Carrier& carrier = found->value;
  llvm::IRBuilder<>& builder = get_builder(*native_body);
  if ((carrier.kind == Kind::Value || carrier.kind == Kind::Enumeration) &&
      elements.get_size() == 1) {
    llvm::Value* source = llvm::unwrap(elements[0]);
    auto* source_integer = llvm::dyn_cast<llvm::IntegerType>(source->getType());
    auto* target_integer = llvm::dyn_cast<llvm::IntegerType>(&native_type);
    if (source_integer && target_integer) {
      return llvm::wrap(
          builder.CreateIntCast(source, target_integer, bool(is_signed(type))));
    }

    if (source->getType()->isFloatingPointTy() &&
        native_type.isFloatingPointTy()) {
      return llvm::wrap(builder.CreateFPCast(source, &native_type));
    }

    fail_toolchain(
        get_program(body),
        "LLVM cannot convert one semantically fitted scalar carrier."_view);
    return {};
  } else if (carrier.kind == Kind::Option && carrier.element) {
    if (elements.get_size() == 0) {
      return zero(get_program(body), type);
    }

    auto payload = assemble(body, *carrier.element, elements);
    if (!payload || !native_body->acquire(*carrier.element, *payload)) {
      return {};
    }

    if (is_object(*carrier.element)) {
      native_body->mark_owned(type, *payload);
      return payload;
    }

    llvm::Value& aggregate = *llvm::UndefValue::get(&native_type);
    llvm::Value& with_payload =
        *builder.CreateInsertValue(&aggregate, llvm::unwrap(*payload), U32(0));
    llvm::Value& selected =
        *builder.CreateInsertValue(&with_payload, builder.getTrue(), U32(1));
    native_body->mark_owned(type, llvm::wrap(&selected));
    return llvm::wrap(&selected);
  } else if (carrier.kind == Kind::Result) {
    fail_toolchain(
        get_program(body),
        "LLVM Result assembly requires one explicitly selected alternative."_view);
    return {};
  } else if (
      carrier.kind == Kind::Object || carrier.kind == Kind::ObjectStorage ||
      carrier.kind == Kind::Implementation) {
    fail_toolchain(
        get_program(body),
        "LLVM cannot assemble a reference carrier from inline payload values."_view);
    return {};
  } else if (carrier.kind == Kind::Fixed && carrier.element) {
    if (elements.get_size() != carrier.extent) {
      fail_toolchain(
          get_program(body),
          "LLVM cannot align Fixed values with its exact extent."_view);
      return {};
    }

    llvm::Value* aggregate = llvm::UndefValue::get(&native_type);
    for (Count index = 0; index < elements.get_size(); index++) {
      Core::View::Vector<LLVMValueRef> selected(elements.get_data() + index, 1);
      auto element = assemble(body, *carrier.element, selected);
      if (!element || !native_body->acquire(*carrier.element, *element)) {
        return {};
      }

      aggregate = builder.CreateInsertValue(
          aggregate, llvm::unwrap(*element), U32(index));
    }

    native_body->mark_owned(type, llvm::wrap(aggregate));
    return llvm::wrap(aggregate);
  } else if (carrier.kind == Kind::Structure) {
    if (!carrier.fields || elements.get_size() != carrier.fields->get_size()) {
      fail_toolchain(
          get_program(body),
          "LLVM cannot align Structure values with its instance Fields."_view);
      return {};
    }

    llvm::Value* aggregate = llvm::UndefValue::get(&native_type);
    for (Count index = 0; index < elements.get_size(); index++) {
      auto field_type = select_field_type(*carrier.fields, index);
      if (!field_type) {
        fail_toolchain(
            get_program(body),
            "LLVM cannot assemble a Structure with an invalid Field edge."_view);
        return {};
      }

      Core::View::Vector<LLVMValueRef> selected(elements.get_data() + index, 1);
      auto field = assemble(body, *field_type, selected);
      if (!field || !native_body->acquire(*field_type, *field)) {
        return {};
      }

      aggregate = builder.CreateInsertValue(
          aggregate, llvm::unwrap(*field), U32(index));
    }

    native_body->mark_owned(type, llvm::wrap(aggregate));
    return llvm::wrap(aggregate);
  }

  llvm::Value* aggregate = llvm::UndefValue::get(&native_type);
  for (Count index = 0; index < elements.get_size(); index++) {
    if (native_type.isSingleValueType()) {
      fail_toolchain(
          get_program(body),
          "LLVM cannot assemble one scalar carrier from multiple values."_view);
      return {};
    }

    aggregate = builder.CreateInsertValue(
        aggregate, llvm::unwrap(elements[index]), U32(index));
  }

  return llvm::wrap(aggregate);
}

auto Llvm::Module::Carriers::fit_values(
    Llvm::Module::Emission& body,
    const Library::Language::Model::Pack& source,
    const Tetrodotoxin::Source::Layout& target,
    Core::View::Vector<LLVMValueRef> values) const
    -> Core::Option<Memory::Dynamic::Vector<LLVMValueRef>> {
  const Tetrodotoxin::Source::Layout& supplied = source.get_layout();
  if (values.get_size() != supplied.get_size() ||
      values.get_size() != target.get_size()) {
    fail_toolchain(
        get_program(body),
        "LLVM cannot align a completed Pack with its fitted target size."_view);
    return {};
  }

  Bool named = False;
  for (Count index = 0; index < supplied.get_size(); index++) {
    if (supplied.get_name(index)) {
      named = True;
      break;
    }
  }

  Memory::Dynamic::Vector<LLVMValueRef> fitted(values.get_size());
  if (!named) {
    for (LLVMValueRef value : values) {
      fitted.insert(value);
    }

    return fitted;
  }

  for (Count target_index = 0; target_index < target.get_size();
       target_index++) {
    Count selected = 0;
    Count matches = 0;
    for (Count source_index = 0; source_index < values.get_size();
         source_index++) {
      if (supplied.fits_entry(target, source_index, target_index)) {
        selected = source_index;
        matches++;
      }
    }

    if (matches != 1) {
      fail_toolchain(
          get_program(body),
          "LLVM cannot select one producer for a fitted target entry."_view);
      return {};
    }

    fitted.insert(values[selected]);
  }

  return fitted;
}

auto Llvm::Module::Carriers::fit_and_assemble(
    Llvm::Module::Emission& body,
    const Tetrodotoxin::Source::Type& type,
    const Library::Language::Model::Pack& source,
    Core::View::Vector<LLVMValueRef> elements) const
    -> Core::Option<LLVMValueRef> {
  auto native_body = get_body(body);
  auto found = carriers.find(&type);
  auto native = get_type(type);
  if (!native_body || !found || !native) {
    fail_toolchain(
        get_program(body),
        "LLVM cannot fit values without a completed target carrier."_view);
    return {};
  }

  if (elements.get_size() == 1 &&
      llvm::unwrap(elements[0])->getType() == llvm::unwrap(*native)) {
    return elements[0];
  }

  const Carrier& carrier = found->value;
  if (carrier.kind == Kind::Implementation && elements.get_size() == 1) {
    // Erasure keeps the supplied Object carrier intact and adds only the
    // Projection for this accepted pair. Acquiring the Object here transfers
    // its lifetime into the resulting Implementation value exactly once.
    auto implementation =
        type.select<Tetrodotoxin::Library::Language::Types::Implementation>();
    const Library::Language::Model::Pack& semantic = source;
    auto candidate =
        semantic.get_value_type(0).resolve().select<Tetrodotoxin::Source::Type>();
    auto candidate_native =
        candidate ? get_type(*candidate) : Core::Option<LLVMTypeRef>();
    auto projection = implementation && candidate && candidate_native &&
                              llvm::unwrap(elements[0])->getType() ==
                                  llvm::unwrap(*candidate_native)
                          ? get_implementation_projection(body, *candidate)
                          : Core::Option<LLVMValueRef>();
    if (!projection || !candidate || !implementation->accepts(semantic) ||
        !native_body->acquire(*candidate, elements[0])) {
      return {};
    }

    llvm::IRBuilder<>& builder = get_builder(*native_body);
    llvm::Value* aggregate = llvm::UndefValue::get(llvm::unwrap(*native));
    aggregate =
        builder.CreateInsertValue(aggregate, llvm::unwrap(elements[0]), U32(0));
    aggregate =
        builder.CreateInsertValue(aggregate, llvm::unwrap(*projection), U32(1));
    native_body->mark_owned(type, llvm::wrap(aggregate));
    return llvm::wrap(aggregate);
  }

  if (carrier.kind == Kind::Fixed && elements.get_size() == carrier.extent) {
    return assemble(body, type, elements);
  }

  if (carrier.kind == Kind::Option && carrier.element) {
    if (elements.get_size() == 0) {
      return assemble(body, type, elements);
    }

    auto payload_type = get_type(*carrier.element);
    if (elements.get_size() == 1 && payload_type &&
        llvm::unwrap(elements[0])->getType() == llvm::unwrap(*payload_type)) {
      return assemble(body, type, elements);
    }

    auto fitted =
        fit_values(body, source, carrier.element->get_layout(), elements);
    return fitted ? assemble(body, type, fitted->get_view())
                  : Core::Option<LLVMValueRef>();
  }

  if (carrier.kind == Kind::Result && carrier.element && carrier.error) {
    Bool value = source.fits_into(*carrier.element);
    Bool error = source.fits_into(*carrier.error);
    if (value == error) {
      fail_toolchain(
          get_program(body),
          "LLVM cannot select exactly one Result alternative for received flow."_view);
      return {};
    }

    const Tetrodotoxin::Source::Type& alternative =
        value ? *carrier.element : *carrier.error;
    auto native_alternative = get_type(alternative);
    if (elements.get_size() == 1 && native_alternative &&
        llvm::unwrap(elements[0])->getType() ==
            llvm::unwrap(*native_alternative)) {
      return assemble_result(body, type, alternative, value, elements);
    }

    auto fitted = fit_values(body, source, alternative.get_layout(), elements);
    return fitted ? assemble_result(
                        body, type, alternative, value, fitted->get_view())
                  : Core::Option<LLVMValueRef>();
  }

  auto fitted = fit_values(body, source, type.get_layout(), elements);
  return fitted ? assemble(body, type, fitted->get_view())
                : Core::Option<LLVMValueRef>();
}

auto Llvm::Module::Carriers::fit(
    Llvm::Module::Emission& body,
    const Library::Language::Model::Pack& source,
    const Tetrodotoxin::Source::Layout& target,
    Core::View::Vector<LLVMValueRef> values) const
    -> Core::Option<Memory::Dynamic::Vector<LLVMValueRef>> {
  return fit_values(body, source, target, values);
}

auto Llvm::Module::Carriers::get_object_descriptor(
    Llvm::Module::Emission& program,
    const Tetrodotoxin::Source::Type& type) const -> Core::Option<LLVMValueRef> {
  auto found = carriers.find(&type);
  if (!found || found->value.phase != Phase::Complete) {
    fail_toolchain(
        program,
        "LLVM cannot select an Object descriptor before carrier completion."_view);
    return {};
  }

  Carrier& carrier = found->value;
  auto target = get_target(program);
  if (!target) {
    return {};
  }

  auto binding = target->get_unit().find_type(type);
  Bool imported = Bool(
      binding && (binding->get_package() != target->get_unit().get_package() ||
                  binding->get_member() != target->get_unit().get_member()));
  if (imported) {
    if (!carrier.descriptor) {
      llvm::Type& count = *llvm::Type::getInt64Ty(get_context(*target));
      llvm::Type& pointer = *llvm::PointerType::getUnqual(get_context(*target));
      llvm::StructType& descriptor_type = *llvm::StructType::get(
          get_context(*target), {&count, &count, &pointer});
      carrier.descriptor = llvm::wrap(new llvm::GlobalVariable(
          get_module(*target), &descriptor_type, true,
          llvm::GlobalValue::ExternalLinkage, nullptr,
          llvm_text(object_descriptor_name(*target, type))));
    }
    return carrier.descriptor;
  }

  if (carrier.kind == Kind::ObjectStorage) {
    auto native_element = carrier.element ? get_type(*carrier.element)
                                          : Core::Option<LLVMTypeRef>();
    if (!native_element) {
      fail_toolchain(
          program,
          "LLVM cannot describe Object[T] before its element carrier."_view);
      return {};
    }

    if (!carrier.descriptor) {
      llvm::Type& count = *llvm::Type::getInt64Ty(get_context(*target));
      llvm::Type& pointer = *llvm::PointerType::getUnqual(get_context(*target));
      llvm::StructType& descriptor_type = *llvm::StructType::get(
          get_context(*target), {&count, &count, &pointer});
      const llvm::DataLayout& layout = get_module(*target).getDataLayout();
      llvm::Constant& size = *llvm::ConstantInt::get(
          &count, layout.getTypeAllocSize(llvm::unwrap(*native_element))
                      .getFixedValue());
      llvm::Constant& alignment = *llvm::ConstantInt::get(
          &count,
          layout.getABITypeAlign(llvm::unwrap(*native_element)).value());
      llvm::FunctionType& finalizer_type = *llvm::FunctionType::get(
          llvm::Type::getVoidTy(get_context(*target)), {&pointer}, false);
      llvm::Constant& finalizer = *llvm::cast<llvm::Constant>(
          get_module(*target)
              .getOrInsertFunction(
                  llvm_text(
                      Perimortem::Abi::Core::object_finalize_trivial_symbol),
                  &finalizer_type)
              .getCallee());
      llvm::Constant& descriptor_value = *llvm::ConstantStruct::get(
          &descriptor_type, {&size, &alignment, &finalizer});
      carrier.descriptor = llvm::wrap(new llvm::GlobalVariable(
          get_module(*target), &descriptor_type, true,
          object_descriptor_linkage(*target, type), &descriptor_value,
          llvm_text(object_descriptor_name(*target, type))));
    }

    return carrier.descriptor;
  }

  if (carrier.kind != Kind::Object || !carrier.payload || !carrier.fields) {
    fail_toolchain(
        program, "LLVM selected a descriptor for a non-Object carrier."_view);
    return {};
  }

  if (!carrier.finalizer) {
    Tetrodotoxin::Terminal::Abi::Representation::Name name(
        target->get_arena(), type,
        Tetrodotoxin::Terminal::Abi::Representation::Name::Kind::
            ObjectFinalizer);
    llvm::FunctionType& signature = *llvm::FunctionType::get(
        llvm::Type::getVoidTy(get_context(*target)),
        {llvm::PointerType::getUnqual(get_context(*target))}, false);
    carrier.finalizer = llvm::wrap(
        llvm::Function::Create(
            &signature, llvm::GlobalValue::InternalLinkage,
            llvm_text(name.get_view()), get_module(*target)));
  }

  llvm::Function& finalizer =
      *llvm::cast<llvm::Function>(llvm::unwrap(*carrier.finalizer));
  if (finalizer.empty()) {
    Body body(*target, type, llvm::wrap(&finalizer));
    llvm::IRBuilder<>& builder = get_builder(body);
    llvm::BasicBlock& entry =
        *llvm::BasicBlock::Create(finalizer.getContext(), "entry", &finalizer);
    builder.SetInsertPoint(&entry);
    llvm::Value& payload = *finalizer.getArg(0);
    for (Count index = carrier.fields->get_size(); index != 0; index--) {
      auto field = select_field_type(*carrier.fields, index - 1);
      if (!field) {
        fail_toolchain(
            program,
            "LLVM cannot finalize an Object with an invalid Field edge."_view);
        return {};
      }

      if (!owns_resources(*field)) {
        continue;
      }

      auto native = get_type(*field);
      if (!native) {
        fail_toolchain(
            program,
            "LLVM cannot emit an Object finalizer without every Field carrier."_view);
        return {};
      }

      llvm::Value& address = *builder.CreateStructGEP(
          llvm::unwrap(*carrier.payload), &payload, U32(index - 1));
      llvm::Value& value = *builder.CreateLoad(llvm::unwrap(*native), &address);
      if (!release(body, *field, llvm::wrap(&value))) {
        return {};
      }
    }

    builder.CreateRetVoid();
  }

  if (!carrier.descriptor) {
    llvm::Type& count = *llvm::Type::getInt64Ty(get_context(*target));
    llvm::Type& pointer = *llvm::PointerType::getUnqual(get_context(*target));
    llvm::StructType& descriptor_type = *llvm::StructType::get(
        get_context(*target), {&count, &count, &pointer});
    const llvm::DataLayout& layout = get_module(*target).getDataLayout();
    llvm::Constant& size = *llvm::ConstantInt::get(
        &count, layout.getTypeAllocSize(llvm::unwrap(*carrier.payload))
                    .getFixedValue());
    llvm::Constant& alignment = *llvm::ConstantInt::get(
        &count, layout.getABITypeAlign(llvm::unwrap(*carrier.payload)).value());
    llvm::Constant& finalizer_pointer = finalizer;
    llvm::Constant& descriptor_value = *llvm::ConstantStruct::get(
        &descriptor_type, {&size, &alignment, &finalizer_pointer});
    carrier.descriptor = llvm::wrap(new llvm::GlobalVariable(
        get_module(*target), &descriptor_type, true,
        object_descriptor_linkage(*target, type), &descriptor_value,
        llvm_text(object_descriptor_name(*target, type))));
  }

  return carrier.descriptor;
}

auto Llvm::Module::Carriers::construct(
    Llvm::Module::Emission& body,
    const Tetrodotoxin::Source::Type& type,
    Core::View::Vector<LLVMValueRef> values) const
    -> Core::Option<LLVMValueRef> {
  auto native_body = get_body(body);
  auto found = carriers.find(&type);
  if (!found || found->value.kind != Kind::Object) {
    return assemble(body, type, values);
  }

  Carrier& carrier = found->value;
  auto target = get_target(get_program(body));
  auto descriptor = get_object_descriptor(get_program(body), type);
  if (!native_body || !target || !carrier.payload || !descriptor ||
      !carrier.fields || values.get_size() != carrier.fields->get_size()) {
    fail_toolchain(
        get_program(body),
        "LLVM cannot pair an Object initializer with its payload Layout."_view);
    return {};
  }

  llvm::FunctionType& signature = *llvm::FunctionType::get(
      llvm::PointerType::getUnqual(get_context(*target)),
      {llvm::PointerType::getUnqual(get_context(*target))}, false);
  llvm::IRBuilder<>& builder = get_builder(*native_body);
  llvm::Value& payload = *builder.CreateCall(
      get_module(*target).getOrInsertFunction(
          llvm_text(Perimortem::Abi::Core::object_allocate_symbol), &signature),
      {llvm::unwrap(*descriptor)}, "object");
  for (Count index = 0; index < carrier.fields->get_size(); index++) {
    auto field_type = select_field_type(*carrier.fields, index);
    if (!field_type) {
      fail_toolchain(
          get_program(body),
          "LLVM cannot construct an Object with an invalid Field edge."_view);
      return {};
    }

    Core::View::Vector<LLVMValueRef> selected(values.get_data() + index, 1);
    auto field = assemble(body, *field_type, selected);
    if (!field || !native_body->acquire(*field_type, *field)) {
      return {};
    }

    llvm::Value& address = *builder.CreateStructGEP(
        llvm::unwrap(*carrier.payload), &payload, U32(index));
    builder.CreateStore(llvm::unwrap(*field), &address);
  }

  native_body->mark_owned(type, llvm::wrap(&payload));
  return llvm::wrap(&payload);
}
