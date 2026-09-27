// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

// LLVM must enter before Perimortem so the standard placement declaration is
// visible before the freestanding fallback used by Perimortem headers.
#if __has_include("llvm/IR/IRBuilder.h")
#include "llvm/IR/IRBuilder.h"
#else
#error LLVM IRBuilder is required by the Library native compiler
#endif

#include "perimortem/core/static/vector.hpp"

#include "llvm-c/Core.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"
#include "perimortem/abi/core/object.hpp"
#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/types/implementation.hpp"
#include "tetrodotoxin/terminal/llvm/emission/storage.hpp"
#include "tetrodotoxin/terminal/llvm/module/carriers.hpp"
#include "tetrodotoxin/terminal/llvm/module/functions.hpp"
#include "tetrodotoxin/terminal/llvm/module/globals.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Terminal;
using namespace Tetrodotoxin::Library;

struct ContiguousSelection {
  llvm::Value& data;
  llvm::Value& length;
};

static auto release_owned(
    Llvm::Module::Body& body,
    const Llvm::Module::Carriers& carriers,
    const Tetrodotoxin::Source::Type& type,
    LLVMValueRef value) -> Bool {
  return !body.take_owned(value) || carriers.release(body, type, value);
}

// Sequence operations preserve contiguous pointer and length carriers while
// semantic owners retain scalar or ranged selection state.

static auto sequence_select_carriers(const Llvm::Module::Body& body)
    -> Core::Option<const Llvm::Module::Carriers&> {
  return body.get_program().get_carriers();
}

static auto sequence_native_builder(const Llvm::Module::Body& body)
    -> llvm::IRBuilder<>& {
  return *reinterpret_cast<llvm::IRBuilder<>*>(body.get_builder());
}

static auto sequence_native_function(const Llvm::Module::Body& body)
    -> llvm::Function& {
  return *llvm::unwrap<llvm::Function>(body.get_function());
}

static auto sequence_find_value(
    const Llvm::Module::Body& body,
    const Tetrodotoxin::Library::Language::Model::Pack& pack)
    -> Core::Option<llvm::Value&> {
  auto found = body.find_value(pack);
  return found ? Core::Option<llvm::Value&>(*llvm::unwrap(*found))
               : Core::Option<llvm::Value&>();
}

static auto select_contiguous_value(
    Llvm::Module::Body& body,
    const Tetrodotoxin::Library::Language::Model::Pack& receiver)
    -> Core::Option<ContiguousSelection> {
  auto value = sequence_find_value(body, receiver);
  if (!value) {
    return {};
  }

  llvm::IRBuilder<>& builder = sequence_native_builder(body);
  auto array = llvm::dyn_cast<llvm::ArrayType>(value->getType());
  if (array) {
    LLVMValueRef storage_handle =
        body.create_entry_alloca(llvm::wrap(array), "sequence.storage"_view);
    llvm::Value& storage = *llvm::unwrap(storage_handle);
    builder.CreateStore(&*value, &storage);
    llvm::Value& data = *builder.CreateInBoundsGEP(
        array, &storage, {builder.getInt64(0), builder.getInt64(0)});
    llvm::Value& length = *builder.getInt64(array->getNumElements());
    return ContiguousSelection{data, length};
  }

  auto structure = llvm::dyn_cast<llvm::StructType>(value->getType());
  if (!structure || structure->getNumElements() != 2) {
    return {};
  }

  llvm::Value& data = *builder.CreateExtractValue(&*value, 0);
  llvm::Value& length = *builder.CreateExtractValue(&*value, 1);
  return ContiguousSelection{data, length};
}

static auto publish_sequence(
    Llvm::Module::Body& body,
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    LLVMValueRef value) -> Bool {
  Core::Static::Vector<LLVMValueRef, 1> values = {{value}};
  return body.publish_values(result, values.get_view());
}

auto Llvm::Emission::Storage::range(
    const Tetrodotoxin::Source::Type& carrier,
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    const Tetrodotoxin::Library::Language::Model::Pack& start,
    const Tetrodotoxin::Library::Language::Model::Pack& end) const -> Bool {
  Llvm::Module::Body& native_body = body;
  auto carriers = sequence_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto first = sequence_find_value(native_body, start);
  auto last = sequence_find_value(native_body, end);
  if (!first || !last) {
    return False;
  }

  Core::Static::Vector<LLVMValueRef, 2> elements = {{
    llvm::wrap(&*first),
    llvm::wrap(&*last),
  }};
  auto assembled =
      carriers->assemble(native_body, carrier, elements.get_view());
  return assembled && publish_sequence(native_body, result, *assembled);
}

auto Llvm::Emission::Storage::empty_range(
    const Tetrodotoxin::Source::Type& carrier,
    const Tetrodotoxin::Library::Language::Model::Pack& result) const -> Bool {
  Llvm::Module::Body& native_body = body;
  auto carriers = sequence_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto value = carriers->zero(body.get_program(), carrier);
  return value && publish_sequence(native_body, result, *value);
}

auto Llvm::Emission::Storage::select_index(
    const Tetrodotoxin::Source::Type& element,
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    const Tetrodotoxin::Library::Language::Model::Pack& receiver,
    const Tetrodotoxin::Library::Language::Model::Pack& index) const -> Bool {
  Llvm::Module::Body& native_body = body;
  auto carriers = sequence_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto contiguous = select_contiguous_value(native_body, receiver);
  auto first = sequence_find_value(native_body, index);
  auto native_element = carriers->get_type(element);
  if (!contiguous || !first || !native_element) {
    return False;
  }

  return native_body.publish_indexed_target(
      result, element, *native_element, llvm::wrap(&contiguous->data),
      llvm::wrap(&contiguous->length), llvm::wrap(&*first), {});
}

auto Llvm::Emission::Storage::select_range(
    const Tetrodotoxin::Source::Type& element,
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    const Tetrodotoxin::Library::Language::Model::Pack& receiver,
    const Tetrodotoxin::Library::Language::Model::Pack& start,
    const Tetrodotoxin::Library::Language::Model::Pack& count,
    Count size) const -> Bool {
  Llvm::Module::Body& native_body = body;
  auto carriers = sequence_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto contiguous = select_contiguous_value(native_body, receiver);
  auto first = sequence_find_value(native_body, start);
  auto native_count = sequence_find_value(native_body, count);
  auto native_element = carriers->get_type(element);
  if (!contiguous || !first || !native_count || !native_element) {
    return False;
  }

  return native_body.publish_indexed_target(
      result, element, *native_element, llvm::wrap(&contiguous->data),
      llvm::wrap(&contiguous->length), llvm::wrap(&*first), size);
}

auto Llvm::Emission::Storage::begin_slice(
    const Tetrodotoxin::Source::Type& element,
    const Tetrodotoxin::Library::Language::Model::Pack& receiver,
    const Tetrodotoxin::Library::Language::Model::Pack& index) const
    -> Core::Option<Choice> {
  Llvm::Module::Body& native_body = body;
  auto carriers = sequence_select_carriers(body);
  if (!carriers) {
    return {};
  }

  auto contiguous = select_contiguous_value(native_body, receiver);
  auto first = sequence_find_value(native_body, index);
  auto native_element = carriers->get_type(element);
  if (!contiguous || !first || !native_element) {
    return {};
  }

  llvm::IRBuilder<>& builder = sequence_native_builder(native_body);
  llvm::Function& function = sequence_native_function(native_body);
  llvm::BasicBlock& selected_block = *llvm::BasicBlock::Create(
      function.getContext(), "slice.selected", &function);
  llvm::BasicBlock& missing_block = *llvm::BasicBlock::Create(
      function.getContext(), "slice.missing", &function);
  llvm::BasicBlock& merge = *llvm::BasicBlock::Create(
      function.getContext(), "slice.merge", &function);
  llvm::Value& in_bounds = *builder.CreateICmpULT(&*first, &contiguous->length);
  builder.CreateCondBr(&in_bounds, &selected_block, &missing_block);

  builder.SetInsertPoint(&selected_block);
  llvm::Value& address = *builder.CreateGEP(
      llvm::unwrap(*native_element), &contiguous->data, &*first);
  llvm::Value& selected =
      *builder.CreateLoad(llvm::unwrap(*native_element), &address);
  if (!carriers->retain(native_body, element, llvm::wrap(&selected))) {
    return {};
  }

  builder.CreateBr(&merge);
  llvm::BasicBlock* selected_end = builder.GetInsertBlock();
  if (!selected_end) {
    return {};
  }

  builder.SetInsertPoint(&missing_block);
  return Choice(
      llvm::wrap(&selected), llvm::wrap(selected_end), llvm::wrap(&merge));
}

auto Llvm::Emission::Storage::end_slice(
    Choice state,
    const Tetrodotoxin::Source::Type& element,
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    const Tetrodotoxin::Library::Language::Model::Pack& fallback) const
    -> Bool {
  Llvm::Module::Body& native_body = body;
  auto carriers = sequence_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto fallback_values = native_body.find_values(fallback);
  if (!fallback_values) {
    return False;
  }

  auto default_value = carriers->fit_and_assemble(
      native_body, element, fallback, fallback_values->get_view());
  if (!default_value || !native_body.acquire(element, *default_value)) {
    return False;
  }

  llvm::IRBuilder<>& builder = sequence_native_builder(native_body);
  builder.CreateBr(llvm::unwrap(state.get_merge()));
  llvm::BasicBlock* default_end = builder.GetInsertBlock();
  if (!default_end) {
    return False;
  }

  builder.SetInsertPoint(llvm::unwrap(state.get_merge()));
  llvm::PHINode& selected =
      *builder.CreatePHI(llvm::unwrap(*default_value)->getType(), 2);
  selected.addIncoming(
      llvm::unwrap(state.get_selected()),
      llvm::unwrap(state.get_selected_end()));
  selected.addIncoming(llvm::unwrap(*default_value), default_end);
  LLVMValueRef selected_handle = llvm::wrap(&selected);
  native_body.mark_owned(element, selected_handle);
  return publish_sequence(native_body, result, selected_handle);
}

auto Llvm::Emission::Storage::begin_slice_range(
    const Tetrodotoxin::Source::Type& element,
    const Tetrodotoxin::Library::Language::Model::Pack& receiver,
    const Tetrodotoxin::Library::Language::Model::Pack& start) const
    -> Core::Option<SliceRange> {
  Llvm::Module::Body& native_body = body;
  auto carriers = sequence_select_carriers(body);
  if (!carriers || !carriers->get_type(element)) {
    return {};
  }

  auto contiguous = select_contiguous_value(native_body, receiver);
  auto first = sequence_find_value(native_body, start);
  if (!contiguous || !first) {
    return {};
  }

  return SliceRange(
      element, llvm::wrap(&contiguous->data), llvm::wrap(&contiguous->length),
      llvm::wrap(&*first));
}

auto Llvm::Emission::Storage::begin_slice_slot(
    const SliceRange& range,
    Count offset) const -> Core::Option<Choice> {
  Llvm::Module::Body& native_body = body;
  auto carriers = sequence_select_carriers(body);
  if (!carriers) {
    return {};
  }

  auto native_element = carriers->get_type(range.get_element());
  if (!native_element) {
    return {};
  }

  llvm::IRBuilder<>& builder = sequence_native_builder(native_body);
  llvm::Function& function = sequence_native_function(native_body);
  llvm::Value& native_offset = *llvm::ConstantInt::get(
      llvm::unwrap(range.get_first())->getType(), offset);
  llvm::Value& index =
      *builder.CreateAdd(llvm::unwrap(range.get_first()), &native_offset);
  llvm::Value& in_bounds =
      *builder.CreateICmpULT(&index, llvm::unwrap(range.get_length()));
  llvm::BasicBlock& selected_block = *llvm::BasicBlock::Create(
      function.getContext(), "slice.selected", &function);
  llvm::BasicBlock& missing_block = *llvm::BasicBlock::Create(
      function.getContext(), "slice.missing", &function);
  llvm::BasicBlock& merge = *llvm::BasicBlock::Create(
      function.getContext(), "slice.merge", &function);
  builder.CreateCondBr(&in_bounds, &selected_block, &missing_block);

  builder.SetInsertPoint(&selected_block);
  llvm::Value& address = *builder.CreateGEP(
      llvm::unwrap(*native_element), llvm::unwrap(range.get_data()), &index);
  llvm::Value& selected =
      *builder.CreateLoad(llvm::unwrap(*native_element), &address);
  if (!carriers->retain(
          native_body, range.get_element(), llvm::wrap(&selected))) {
    return {};
  }

  builder.CreateBr(&merge);
  llvm::BasicBlock* selected_end = builder.GetInsertBlock();
  if (!selected_end) {
    return {};
  }

  builder.SetInsertPoint(&missing_block);
  return Choice(
      llvm::wrap(&selected), llvm::wrap(selected_end), llvm::wrap(&merge));
}

auto Llvm::Emission::Storage::end_slice_slot(
    Choice state,
    const Tetrodotoxin::Source::Type& element,
    const Tetrodotoxin::Library::Language::Model::Pack& fallback) const
    -> Core::Option<LLVMValueRef> {
  Llvm::Module::Body& native_body = body;
  auto carriers = sequence_select_carriers(body);
  if (!carriers) {
    return {};
  }

  auto fallback_values = native_body.find_values(fallback);
  if (!fallback_values) {
    return {};
  }

  auto default_value = carriers->fit_and_assemble(
      native_body, element, fallback, fallback_values->get_view());
  if (!default_value || !native_body.acquire(element, *default_value)) {
    return {};
  }

  llvm::IRBuilder<>& builder = sequence_native_builder(native_body);
  builder.CreateBr(llvm::unwrap(state.get_merge()));
  llvm::BasicBlock* default_end = builder.GetInsertBlock();
  if (!default_end) {
    return {};
  }

  builder.SetInsertPoint(llvm::unwrap(state.get_merge()));
  llvm::PHINode& selected =
      *builder.CreatePHI(llvm::unwrap(*default_value)->getType(), 2);
  selected.addIncoming(
      llvm::unwrap(state.get_selected()),
      llvm::unwrap(state.get_selected_end()));
  selected.addIncoming(llvm::unwrap(*default_value), default_end);
  LLVMValueRef selected_value = llvm::wrap(&selected);
  native_body.mark_owned(element, selected_value);
  return selected_value;
}

auto Llvm::Emission::Storage::end_slice_range(
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    Core::View::Vector<LLVMValueRef> values) const -> Bool {
  Llvm::Module::Body& native_body = body;
  return native_body.publish_values(result, values);
}

// Storage selection maps exact Addressable and Pack identities to local,
// global, member, or indexed native addresses.

class StorageSelection {
 public:
  constexpr StorageSelection(
      Llvm::Module::Body& body,
      Llvm::Module::Program& program,
      const Llvm::Module::Carriers& carriers)
      : body(body), program(program), carriers(carriers) {}

  Llvm::Module::Body& body;
  Llvm::Module::Program& program;
  const Llvm::Module::Carriers& carriers;
};

static auto select_storage_value(Llvm::Module::Body& body)
    -> Core::Option<StorageSelection> {
  return StorageSelection(
      body, body.get_program(), body.get_program().get_carriers());
}

static auto select_global_storage(const Llvm::Module::Body& body)
    -> Core::Option<const Llvm::Module::Globals&> {
  return body.get_program().get_globals();
}

static auto publish_storage(
    Llvm::Module::Body& body,
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    LLVMValueRef value) -> Bool {
  Core::Static::Vector<LLVMValueRef, 1> values = {{value}};
  return value && body.publish_values(result, values.get_view());
}

auto Llvm::Emission::Storage::select(
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    const Tetrodotoxin::Source::Addressable& addressable) const -> Bool {
  auto selected = select_storage_value(body);
  if (!selected) {
    return False;
  }

  auto address = selected->body.find_address(addressable);
  if (!address) {
    auto globals = select_global_storage(body);
    if (globals) {
      address = globals->find_address(addressable);
    }
  }

  if (!address) {
    return selected->program.fail_toolchain(
        "LLVM cannot select storage before its exact Addressable publishes an address."_view);
  }

  auto type = addressable.get_type().select<Tetrodotoxin::Source::Type>();
  return type && selected->body.publish_target_address(result, *type, *address);
}

auto Llvm::Emission::Storage::select_member(
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    const Tetrodotoxin::Source::Addressable& addressable,
    const Tetrodotoxin::Library::Language::Model::Pack& receiver) const
    -> Bool {
  auto selected = select_storage_value(body);
  if (!selected) {
    return False;
  }

  auto host = selected->carriers.get_field_host(addressable);
  auto field_index = selected->carriers.get_field_index(addressable);
  if (!host || !field_index) {
    return selected->program.fail_toolchain(
        "LLVM cannot select a member before its host carrier publishes the Field index."_view);
  }

  auto payload = selected->carriers.get_payload(*host);
  if (!payload) {
    return selected->program.fail_toolchain(
        "LLVM cannot address a member without its completed host payload carrier."_view);
  }

  Core::Option<LLVMValueRef> base;
  if (host->is<Tetrodotoxin::Library::Language::Types::Implementation>()) {
    auto value = selected->body.find_value(receiver);
    auto native_host = selected->carriers.get_type(*host);
    if (!value || !native_host || LLVMTypeOf(*value) != *native_host) {
      return selected->program.fail_toolchain(
          "LLVM Interface member access requires one exact Implementation carrier."_view);
    }

    base = LLVMBuildExtractValue(
        selected->body.get_builder(), *value, 0, "implementation.object");
  } else if (selected->carriers.is_object(*host)) {
    auto value = selected->body.find_value(receiver);
    auto native_host = selected->carriers.get_type(*host);
    if (!value || !native_host || LLVMTypeOf(*value) != *native_host) {
      return selected->program.fail_toolchain(
          "LLVM Object member access requires one exact receiver handle."_view);
    }

    base = *value;
  } else {
    auto target = selected->body.find_target_address(receiver);
    if (!target || &target->get_type() != &*host) {
      return selected->program.fail_toolchain(
          "LLVM inline member access requires the receiver's exact storage address."_view);
    }

    base = target->get_address();
  }

  if (!base) {
    return False;
  }

  LLVMValueRef member = LLVMBuildStructGEP2(
      selected->body.get_builder(), *payload, *base, U32(*field_index),
      "member");
  if (!member) {
    return selected->program.fail_toolchain(
        "LLVM could not create the selected member address."_view);
  }

  if (!selected->carriers.is_object(*host)) {
    auto receiver_value = selected->body.find_value(receiver);
    if (receiver_value &&
        !release_owned(
            selected->body, selected->carriers, *host, *receiver_value)) {
      return False;
    }
  }

  auto type = addressable.get_type().select<Tetrodotoxin::Source::Type>();
  return type && selected->body.publish_target_address(result, *type, member);
}

auto Llvm::Emission::Storage::load(
    const Tetrodotoxin::Library::Language::Model::Pack& result) const -> Bool {
  auto selected = select_storage_value(body);
  if (!selected) {
    return False;
  }

  auto target = selected->body.find_target_address(result);
  if (!target) {
    return selected->program.fail_toolchain(
        "LLVM cannot load a Pack before its exact storage target is selected."_view);
  }

  auto native_type = selected->carriers.get_type(target->get_type());
  if (!native_type) {
    return selected->program.fail_toolchain(
        "LLVM cannot load storage without its completed value carrier."_view);
  }

  LLVMValueRef value = LLVMBuildLoad2(
      selected->body.get_builder(), *native_type, target->get_address(),
      "load");
  if (!value ||
      !selected->carriers.retain(selected->body, target->get_type(), value)) {
    return False;
  }

  selected->body.mark_owned(target->get_type(), value);
  return publish_storage(selected->body, result, value);
}

// Value composition follows the Pack's retained child flows. Layout entries
// already identify the real semantic producers, so lowering needs no parallel
// producer map.

auto Llvm::Emission::Storage::compose(
    const Tetrodotoxin::Library::Language::Model::Pack& result) const -> Bool {
  Llvm::Module::Body& native_body = body;
  Memory::Dynamic::Vector<LLVMValueRef> composed;
  for (const Tetrodotoxin::Source::PackReference<
           Tetrodotoxin::Library::Language::Model::Pack>& entry :
       result.get_entries()) {
    auto source = native_body.find_values(entry.get());
    if (!source) {
      return False;
    }
    for (LLVMValueRef value : source->get_view()) {
      composed.insert(value);
    }
  }

  return native_body.publish_values(result, composed.get_view());
}

auto Llvm::Emission::Storage::alias(
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    const Tetrodotoxin::Library::Language::Model::Pack& source) const -> Bool {
  Llvm::Module::Body& native_body = body;
  auto source_values = native_body.find_values(source);
  if (!source_values) {
    return False;
  }

  return native_body.publish_values(result, source_values->get_view());
}

// Writes complete bounds checks before mutation and transfer ownership only
// after the complete target accepts the source value.

class WriteSelection {
 public:
  constexpr WriteSelection(
      Llvm::Module::Body& body,
      Llvm::Module::Program& program,
      const Llvm::Module::Carriers& carriers)
      : body(body), program(program), carriers(carriers) {}

  Llvm::Module::Body& body;
  Llvm::Module::Program& program;
  const Llvm::Module::Carriers& carriers;
};

class WriteBranches {
 public:
  constexpr explicit WriteBranches(LLVMBasicBlockRef done) : done(done) {}

  LLVMBasicBlockRef done;
};

enum class StoreOwnership : U8 {
  ConsumeTemporary,
  RetainTemporary,
};

static auto select_write_target(Llvm::Module::Body& body)
    -> Core::Option<WriteSelection> {
  return WriteSelection(
      body, body.get_program(), body.get_program().get_carriers());
}

static auto publish_write(
    Llvm::Module::Body& body,
    const Tetrodotoxin::Library::Language::Model::Pack& result) -> Bool {
  Core::View::Vector<LLVMValueRef> values;
  return body.publish_values(result, values);
}

static auto replace_written_value(
    WriteSelection& selected,
    const Tetrodotoxin::Source::Type& type,
    LLVMValueRef address,
    LLVMValueRef value,
    StoreOwnership ownership) -> Bool {
  auto native_type = selected.carriers.get_type(type);
  if (!native_type || !address || !value || LLVMTypeOf(value) != *native_type) {
    return selected.program.fail_toolchain(
        "LLVM cannot store a value that does not match its exact target carrier."_view);
  }

  Bool acquired = ownership == StoreOwnership::ConsumeTemporary
                      ? selected.body.acquire(type, value)
                      : selected.carriers.retain(selected.body, type, value);
  if (!acquired) {
    return False;
  }

  LLVMValueRef previous = LLVMBuildLoad2(
      selected.body.get_builder(), *native_type, address, "previous");
  if (!previous || !selected.carriers.release(selected.body, type, previous)) {
    return False;
  }

  return Bool(LLVMBuildStore(selected.body.get_builder(), value, address));
}

static auto begin_indexed_store(
    WriteSelection& selected,
    LLVMValueRef first,
    LLVMValueRef length,
    Core::Option<Count> range_size) -> Core::Option<WriteBranches> {
  Core::Option<LLVMValueRef> condition;
  if (!range_size) {
    condition = LLVMBuildICmp(
        selected.body.get_builder(), LLVMIntULT, first, length, "index.valid");
  } else {
    LLVMValueRef begins = LLVMBuildICmp(
        selected.body.get_builder(), LLVMIntULE, first, length,
        "range.start.valid");
    LLVMValueRef remaining = LLVMBuildSub(
        selected.body.get_builder(), length, first, "range.remaining");
    LLVMValueRef count =
        LLVMConstInt(LLVMTypeOf(first), U64(*range_size), LLVMBool(0));
    LLVMValueRef fits = LLVMBuildICmp(
        selected.body.get_builder(), LLVMIntULE, count, remaining,
        "range.count.valid");
    condition =
        LLVMBuildAnd(selected.body.get_builder(), begins, fits, "range.valid");
  }

  LLVMValueRef function = selected.body.get_function();
  LLVMModuleRef module = LLVMGetGlobalParent(function);
  if (!condition || !module) {
    return {};
  }

  LLVMContextRef context = LLVMGetModuleContext(module);
  LLVMBasicBlockRef write =
      LLVMAppendBasicBlockInContext(context, function, "write.selected");
  LLVMBasicBlockRef done =
      LLVMAppendBasicBlockInContext(context, function, "write.done");
  if (!write || !done ||
      !LLVMBuildCondBr(selected.body.get_builder(), *condition, write, done)) {
    return {};
  }

  LLVMPositionBuilderAtEnd(selected.body.get_builder(), write);
  return WriteBranches(done);
}

static auto end_indexed_store(
    WriteSelection& selected,
    const WriteBranches& blocks) -> Bool {
  LLVMBasicBlockRef current = LLVMGetInsertBlock(selected.body.get_builder());
  if (!current) {
    return False;
  }

  if (!LLVMGetBasicBlockTerminator(current) &&
      !LLVMBuildBr(selected.body.get_builder(), blocks.done)) {
    return False;
  }

  LLVMPositionBuilderAtEnd(selected.body.get_builder(), blocks.done);
  return True;
}

static auto get_indexed_address(
    WriteSelection& selected,
    LLVMTypeRef native_type,
    LLVMValueRef data,
    LLVMValueRef first,
    Count offset) -> Core::Option<LLVMValueRef> {
  LLVMValueRef index = first;
  if (offset != 0) {
    LLVMValueRef displacement =
        LLVMConstInt(LLVMTypeOf(first), U64(offset), LLVMBool(0));
    index = LLVMBuildAdd(
        selected.body.get_builder(), first, displacement, "write.index");
  }

  LLVMValueRef address = LLVMBuildGEP2(
      selected.body.get_builder(), native_type, data, &index, 1,
      "write.address");
  return address ? Core::Option<LLVMValueRef>(address)
                 : Core::Option<LLVMValueRef>();
}

static auto assign_to_address(
    WriteSelection& selected,
    const Tetrodotoxin::Library::Language::Model::Pack& target,
    const Tetrodotoxin::Library::Language::Model::Pack& source) -> Bool {
  auto address = selected.body.find_target_address(target);
  auto values = selected.body.find_values(source);
  if (!address || !values) {
    return False;
  }

  auto stored = selected.carriers.fit_and_assemble(
      selected.body, address->get_type(), source, values->get_view());
  return stored && replace_written_value(
                       selected, address->get_type(), address->get_address(),
                       *stored, StoreOwnership::ConsumeTemporary);
}

static auto assign_to_index(
    WriteSelection& selected,
    const Tetrodotoxin::Library::Language::Model::Pack& target,
    const Tetrodotoxin::Library::Language::Model::Pack& source) -> Bool {
  auto indexed = selected.body.find_indexed_target(target);
  auto values = selected.body.find_values(source);
  if (!indexed || !values) {
    return False;
  }

  Count count = indexed->get_range_size().visit(
      []() -> Count { return 1; },
      [](Count selected) -> Count { return selected; });
  if (values->get_size() != count) {
    return selected.program.fail_toolchain(
        "LLVM indexed assignment did not receive the exact selected value count."_view);
  }

  Memory::Dynamic::Vector<LLVMValueRef> stored_values(count);
  for (Count offset = 0; offset < count; offset++) {
    LLVMValueRef source_value = values->get_data()[offset];
    Core::Static::Vector<LLVMValueRef, 1> elements = {{source_value}};
    auto stored =
        indexed->get_range_size()
            ? selected.carriers.assemble(
                  selected.body, indexed->get_type(), elements.get_view())
            : selected.carriers.fit_and_assemble(
                  selected.body, indexed->get_type(), source,
                  elements.get_view());
    if (!stored) {
      return False;
    }

    stored_values.insert(*stored);
  }

  auto blocks = begin_indexed_store(
      selected, indexed->get_first(), indexed->get_length(),
      indexed->get_range_size());
  if (!blocks) {
    return False;
  }

  for (Count offset = 0; offset < count; offset++) {
    auto address = get_indexed_address(
        selected, indexed->get_native_type(), indexed->get_data(),
        indexed->get_first(), offset);
    if (!address ||
        !replace_written_value(
            selected, indexed->get_type(), *address, stored_values[offset],
            StoreOwnership::RetainTemporary)) {
      return False;
    }
  }

  return end_indexed_store(selected, *blocks);
}

static auto create_compound_result(
    WriteSelection& selected,
    const Tetrodotoxin::Source::Type& type,
    LLVMValueRef left,
    LLVMValueRef right,
    Llvm::Emission::Storage::Write operation) -> LLVMValueRef {
  if (selected.carriers.is_real(type)) {
    if (operation == Llvm::Emission::Storage::Write::Subtract) {
      return LLVMBuildFSub(
          selected.body.get_builder(), left, right, "compound.value");
    }

    return LLVMBuildFAdd(
        selected.body.get_builder(), left, right, "compound.value");
  }

  if (operation == Llvm::Emission::Storage::Write::Subtract) {
    return LLVMBuildSub(
        selected.body.get_builder(), left, right, "compound.value");
  }

  return LLVMBuildAdd(
      selected.body.get_builder(), left, right, "compound.value");
}

static auto compound_to_address(
    WriteSelection& selected,
    const Tetrodotoxin::Library::Language::Model::Pack& target,
    const Tetrodotoxin::Library::Language::Model::Pack& right,
    Llvm::Emission::Storage::Write operation) -> Bool {
  auto address = selected.body.find_target_address(target);
  auto right_value = selected.body.find_value(right);
  if (!address || !right_value) {
    return False;
  }

  auto native_type = selected.carriers.get_type(address->get_type());
  if (!native_type || LLVMTypeOf(*right_value) != *native_type) {
    return selected.program.fail_toolchain(
        "LLVM compound assignment requires the right value's exact target carrier."_view);
  }

  LLVMValueRef left = LLVMBuildLoad2(
      selected.body.get_builder(), *native_type, address->get_address(),
      "compound.left");
  LLVMValueRef value = create_compound_result(
      selected, address->get_type(), left, *right_value, operation);
  return replace_written_value(
      selected, address->get_type(), address->get_address(), value,
      StoreOwnership::ConsumeTemporary);
}

static auto compound_to_index(
    WriteSelection& selected,
    const Tetrodotoxin::Library::Language::Model::Pack& target,
    const Tetrodotoxin::Library::Language::Model::Pack& right,
    Llvm::Emission::Storage::Write operation) -> Bool {
  auto indexed = selected.body.find_indexed_target(target);
  auto right_value = selected.body.find_value(right);
  if (!indexed || indexed->get_range_size() || !right_value ||
      LLVMTypeOf(*right_value) != indexed->get_native_type()) {
    return False;
  }

  auto blocks = begin_indexed_store(
      selected, indexed->get_first(), indexed->get_length(), {});
  auto address = blocks ? get_indexed_address(
                              selected, indexed->get_native_type(),
                              indexed->get_data(), indexed->get_first(), 0)
                        : Core::Option<LLVMValueRef>();
  if (!blocks || !address) {
    return False;
  }

  LLVMValueRef left = LLVMBuildLoad2(
      selected.body.get_builder(), indexed->get_native_type(), *address,
      "compound.left");
  LLVMValueRef value = create_compound_result(
      selected, indexed->get_type(), left, *right_value, operation);
  if (!replace_written_value(
          selected, indexed->get_type(), *address, value,
          StoreOwnership::RetainTemporary)) {
    return False;
  }

  return end_indexed_store(selected, *blocks);
}

auto Llvm::Emission::Storage::write(
    Write operation,
    const Tetrodotoxin::Library::Language::Model::Pack& result,
    const Tetrodotoxin::Library::Language::Model::Pack& target,
    const Tetrodotoxin::Library::Language::Model::Pack& source) const -> Bool {
  auto selected = select_write_target(body);
  if (!selected) {
    return False;
  }

  Bool direct = Bool(selected->body.find_target_address(target));
  Bool indexed = Bool(selected->body.find_indexed_target(target));
  if (!direct && !indexed) {
    return selected->program.fail_toolchain(
        "LLVM write requires one selected direct or indexed target."_view);
  }

  Bool stored = False;
  switch (operation) {
  case Write::Assign:
    stored = direct ? assign_to_address(*selected, target, source)
                    : assign_to_index(*selected, target, source);
    break;

  case Write::Add:
  case Write::Subtract:
    stored = direct ? compound_to_address(*selected, target, source, operation)
                    : compound_to_index(*selected, target, source, operation);
    break;
  }

  return stored && publish_write(selected->body, result);
}
