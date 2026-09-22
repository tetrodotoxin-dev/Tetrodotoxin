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
#include "tetrodotoxin/terminal/llvm/emission/invocation.hpp"
#include "tetrodotoxin/terminal/llvm/emission/storage.hpp"
#include "tetrodotoxin/terminal/llvm/module/carriers.hpp"
#include "tetrodotoxin/terminal/llvm/module/functions.hpp"
#include "tetrodotoxin/terminal/llvm/module/globals.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Terminal;
using namespace Tetrodotoxin::Library;

// Arithmetic validates exact carriers before choosing signed, unsigned, or

static auto call_select_carriers(const Llvm::Module::Body& body)
    -> Core::Option<const Llvm::Module::Carriers&> {
  return body.get_program().get_carriers();
}

static auto call_select_functions(const Llvm::Module::Body& body)
    -> Core::Option<const Llvm::Module::Functions&> {
  return body.get_program().get_functions();
}

static auto call_fail_toolchain(
    Llvm::Module::Body& body,
    Core::View::Bytes message) -> Bool {
  return body.get_program().fail_toolchain(message);
}

static auto call_select_type(const Tetrodotoxin::Source::Abstract& answer)
    -> Core::Option<const Tetrodotoxin::Source::Type&> {
  auto direct = answer.select<Tetrodotoxin::Source::Type>();
  return direct ? direct : answer.resolve().select<Tetrodotoxin::Source::Type>();
}

static auto call_select_result_type(
    const Tetrodotoxin::Source::Layout& layout,
    Count index) -> Core::Option<const Tetrodotoxin::Source::Type&> {
  auto entry = layout.get_abstract(index);
  if (!entry) {
    return {};
  }

  auto addressable = entry->select<Tetrodotoxin::Source::Addressable>();
  if (addressable) {
    return call_select_type(addressable->get_type());
  }

  return call_select_type(*entry);
}

static auto call_select_input_values(
    const Llvm::Module::Body& body,
    const Tetrodotoxin::Source::Pack& source,
    Count offset,
    Count size) -> Core::Option<Core::View::Vector<LLVMValueRef>> {
  auto values = body.find_values(source);
  if (!values || values->get_size() != source.get_layout().get_size() ||
      size == 0 || offset > values->get_size() ||
      size > values->get_size() - offset) {
    return {};
  }

  return Core::View::Vector<LLVMValueRef>(values->get_data() + offset, size);
}

static auto call_assemble_input(
    Llvm::Module::Body& body,
    const Llvm::Module::Carriers& carriers,
    const Tetrodotoxin::Source::Addressable& parameter,
    const Tetrodotoxin::Library::Language::Model::Pack& source,
    Count offset,
    Count size) -> Core::Option<LLVMValueRef> {
  auto values = call_select_input_values(body, source, offset, size);
  if (!values) {
    call_fail_toolchain(
        body,
        "LLVM cannot bind a Callable parameter to its exact source Pack segment."_view);
    return {};
  }

  auto type = call_select_type(parameter.get_type());
  BAIL_IF(!type);
  Bool complete_source =
      Bool(offset == 0 && size == source.get_layout().get_size());
  return complete_source
             ? carriers.fit_and_assemble(body, *type, source, *values)
             : carriers.assemble(body, *type, *values);
}

static auto call_release_owned(
    Llvm::Module::Body& body,
    const Llvm::Module::Carriers& carriers,
    const Tetrodotoxin::Source::Type& type,
    LLVMValueRef value) -> Bool {
  if (!body.take_owned(value)) {
    return True;
  }

  return carriers.release(body, type, value);
}

static auto call_publish_empty(
    Llvm::Module::Body& body,
    const Tetrodotoxin::Source::Pack& result) -> Bool {
  return body.publish_values(result, Core::View::Vector<LLVMValueRef>());
}

static auto call_publish_results(
    Llvm::Module::Body& body,
    const Llvm::Module::Carriers& carriers,
    const Tetrodotoxin::Source::Pack& result,
    const Tetrodotoxin::Source::Layout& signature,
    Core::Option<LLVMOpaqueValue&> returned) -> Bool {
  if (result.get_layout().get_size() != signature.get_size()) {
    return call_fail_toolchain(
        body,
        "LLVM Callable output does not match its completed result signature."_view);
  }

  if (signature.is_empty()) {
    return call_publish_empty(body, result);
  }

  if (!returned) {
    return call_fail_toolchain(
        body,
        "LLVM received no native value for a nonempty Callable result Layout."_view);
  }

  LLVMValueRef native_returned = &*returned;
  if (signature.get_size() == 1) {
    auto type = call_select_result_type(signature, 0);
    auto native = type ? carriers.get_type(*type) : Core::Option<LLVMTypeRef>();
    if (!type || !native || LLVMTypeOf(native_returned) != *native) {
      return call_fail_toolchain(
          body,
          "LLVM received a Callable result with the wrong physical carrier."_view);
    }

    Core::Static::Vector<LLVMValueRef, 1> values = {{native_returned}};
    body.mark_owned(*type, native_returned);
    return body.publish_values(result, values.get_view());
  }

  LLVMTypeRef aggregate = LLVMTypeOf(native_returned);
  if (LLVMGetTypeKind(aggregate) != LLVMStructTypeKind ||
      LLVMCountStructElementTypes(aggregate) != signature.get_size()) {
    return call_fail_toolchain(
        body,
        "LLVM received a multi-result Callable without its exact aggregate carrier."_view);
  }

  Llvm::Module::Body::NativeValues values(signature.get_size());
  for (Count index = 0; index < signature.get_size(); index++) {
    auto type = call_select_result_type(signature, index);
    auto native = type ? carriers.get_type(*type) : Core::Option<LLVMTypeRef>();
    LLVMValueRef value = LLVMBuildExtractValue(
        body.get_builder(), native_returned, U32(index), "call.result");
    if (!type || !native || !value || LLVMTypeOf(value) != *native) {
      return call_fail_toolchain(
          body,
          "LLVM could not extract one exact Callable result carrier."_view);
    }

    body.mark_owned(*type, value);
    values.insert(value);
  }

  return body.publish_values(result, values.get_view());
}

auto Llvm::Emission::Invocation::fit_input(
    const Tetrodotoxin::Source::Addressable& parameter,
    const Library::Language::Model::Pack& source,
    Count offset,
    Count size) const -> Core::Option<LLVMValueRef> {
  auto carriers = call_select_carriers(body);
  return carriers ? call_assemble_input(
                        body, *carriers, parameter, source, offset, size)
                  : Core::Option<LLVMValueRef>();
}

auto Llvm::Emission::Invocation::invoke(
    const Library::Language::Model::Pack& result,
    const Tetrodotoxin::Source::Callable& callable,
    Core::View::Vector<LLVMValueRef> inputs,
    Core::Option<const Tetrodotoxin::Source::Pack&> receiver_source) const -> Bool {
  Llvm::Module::Body& native_body = body;
  auto carriers = call_select_carriers(body);
  auto functions = call_select_functions(body);
  if (!carriers || !functions) {
    return False;
  }

  auto function = functions->find_function(callable);
  Core::View::Vector<Bool> indirect =
      functions->get_indirect_parameters(callable);
  const Tetrodotoxin::Source::Layout& parameters = callable.get_parameters();
  if (!function || inputs.get_size() != parameters.get_size() ||
      indirect.get_size() != inputs.get_size()) {
    return call_fail_toolchain(
        body,
        "LLVM cannot invoke a Callable before its complete physical signature."_view);
  }

  for (Count index = 0; index < inputs.get_size(); index++) {
    auto parameter = parameters.get_abstract(index);
    auto addressable = parameter
                           ? parameter->select<Tetrodotoxin::Source::Addressable>()
                           : Core::Option<const Tetrodotoxin::Source::Addressable&>();
    auto type = addressable ? call_select_type(addressable->get_type())
                            : Core::Option<const Tetrodotoxin::Source::Type&>();
    auto native =
        type ? carriers->get_type(*type) : Core::Option<LLVMTypeRef>();
    if (!addressable || !type || !native ||
        LLVMTypeOf(inputs.get_data()[index]) != *native) {
      return call_fail_toolchain(
          body,
          "LLVM received a Call input with the wrong parameter carrier."_view);
    }
  }

  Llvm::Module::Body::NativeValues native_arguments(inputs.get_size() + 1);
  auto sret_type = functions->find_sret_type(callable);
  Core::Option<LLVMValueRef> returned_storage;
  Core::Option<const Tetrodotoxin::Source::Type&> temporary_self_type;
  Core::Option<LLVMValueRef> temporary_self_address;
  if (sret_type) {
    returned_storage =
        native_body.create_entry_alloca(*sret_type, "call.result"_view);
    if (!returned_storage) {
      return call_fail_toolchain(
          body,
          "LLVM could not reserve the indirect Callable result storage."_view);
    }

    native_arguments.insert(*returned_storage);
  }

  for (Count index = 0; index < inputs.get_size(); index++) {
    LLVMValueRef argument = inputs.get_data()[index];
    if (indirect[index]) {
      auto parameter = parameters.get_abstract(index);
      auto addressable = parameter
                             ? parameter->select<Tetrodotoxin::Source::Addressable>()
                             : Core::Option<const Tetrodotoxin::Source::Addressable&>();
      if (!addressable) {
        return False;
      }

      auto type = call_select_type(addressable->get_type());
      auto native =
          type ? carriers->get_type(*type) : Core::Option<LLVMTypeRef>();
      if (!type || !native) {
        return call_fail_toolchain(
            body,
            "LLVM cannot pass a parameter indirectly without its carrier."_view);
      }

      Bool self_reference =
          Bool(index == 0 && addressable->get_name() == "self"_view);
      Core::Option<LLVMValueRef> selected_address;
      if (self_reference && receiver_source) {
        auto target = native_body.find_target_address(*receiver_source);
        if (target && &target->get_type() == &*type) {
          selected_address = target->get_address();
        }
      }

      LLVMValueRef address =
          selected_address
              ? *selected_address
              : native_body.create_entry_alloca(*native, "call.argument"_view);
      if (!address) {
        return call_fail_toolchain(
            body,
            "LLVM could not reserve one indirect Callable argument."_view);
      }

      if (!selected_address) {
        if (self_reference && carriers->owns_resources(*type) &&
            !native_body.take_owned(argument) &&
            !carriers->retain(native_body, *type, argument)) {
          return False;
        }

        LLVMValueRef stored =
            LLVMBuildStore(native_body.get_builder(), argument, address);
        if (!stored) {
          return call_fail_toolchain(
              body,
              "LLVM could not materialize one indirect Callable argument."_view);
        }

        if (self_reference) {
          temporary_self_type = *type;
          temporary_self_address = address;
        }
      } else if (
          self_reference && carriers->owns_resources(*type) &&
          native_body.take_owned(argument) &&
          !carriers->release(native_body, *type, argument)) {
        return False;
      }

      argument = address;
      native_arguments.insert(argument);
    } else if (!functions->append_call_arguments(
                   body, callable, index, argument, native_arguments)) {
      return False;
    }
  }

  LLVMTypeRef signature = LLVMGlobalGetValueType(*function);
  LLVMTypeRef native_result = LLVMGetReturnType(signature);
  Bool returns_void = Bool(LLVMGetTypeKind(native_result) == LLVMVoidTypeKind);

  LLVMValueRef invoked = LLVMBuildCall2(
      native_body.get_builder(), signature, *function,
      native_arguments.get_data(), U32(native_arguments.get_size()),
      returns_void ? "" : "call");
  if (!invoked) {
    return call_fail_toolchain(
        body, "LLVM could not emit the selected Call."_view);
  }

  const Tetrodotoxin::Source::Layout& results = callable.get_results();
  auto library_callable =
      callable.select<Tetrodotoxin::Library::Language::Model::Callable>();
  auto self_result = library_callable
                         ? library_callable->get_self_result()
                         : Core::Option<const Tetrodotoxin::Source::Addressable&>();
  if (temporary_self_type && temporary_self_address) {
    if (self_result) {
      if (!native_body.register_storage(
              *temporary_self_type, *temporary_self_address)) {
        return call_fail_toolchain(
            body, "LLVM could not retain one returned Self reference."_view);
      }
    } else {
      auto native = carriers->get_type(*temporary_self_type);
      LLVMValueRef value = native
                               ? LLVMBuildLoad2(
                                     native_body.get_builder(), *native,
                                     *temporary_self_address, "self.temporary")
                               : nullptr;
      if (!value ||
          !carriers->release(native_body, *temporary_self_type, value)) {
        return call_fail_toolchain(
            body, "LLVM could not release one temporary Self receiver."_view);
      }
    }
  }

  if (self_result) {
    auto self_type = call_select_type(self_result->get_type());
    if (returns_void || returned_storage || !self_type ||
        LLVMGetTypeKind(LLVMTypeOf(invoked)) != LLVMPointerTypeKind ||
        !native_body.publish_target_address(result, *self_type, invoked)) {
      return call_fail_toolchain(
          body, "LLVM could not publish one returned Self reference."_view);
    }
    return Storage(body).load(result);
  }

  // Callable parameters borrow their inputs. Body keeps an owned temporary
  // through the complete Statement so a returned View can still borrow that
  // storage while the enclosing expression consumes it. Receiving storage or
  // a Return explicitly takes ownership before Statement cleanup.

  if (result.get_layout().get_size() != results.get_size()) {
    return call_fail_toolchain(
        body,
        "LLVM Call Pack does not expose its completed Callable result count."_view);
  }

  if (results.is_empty()) {
    if (!returns_void || returned_storage) {
      return call_fail_toolchain(
          body,
          "LLVM retained a native value for an empty Callable result Layout."_view);
    }

    return call_publish_empty(native_body, result);
  }

  Core::Option<LLVMOpaqueValue&> returned;
  if (returned_storage && sret_type) {
    LLVMValueRef loaded = LLVMBuildLoad2(
        native_body.get_builder(), *sret_type, *returned_storage,
        "call.result");
    if (loaded) {
      returned = *loaded;
    }
  } else if (!returns_void) {
    auto decoded = functions->decode_call_result(body, callable, invoked);
    if (!decoded) {
      return call_fail_toolchain(
          body, "LLVM could not restore one semantic C result."_view);
    }
    returned = **decoded;
  }

  return call_publish_results(
      native_body, *carriers, result, results, returned);
}

auto Llvm::Emission::Invocation::get_size(
    const Tetrodotoxin::Source::Pack& result,
    const Tetrodotoxin::Source::Type& result_type,
    const Tetrodotoxin::Source::Type& receiver_type,
    LLVMValueRef receiver) const -> Bool {
  Llvm::Module::Body& native_body = body;
  auto carriers = call_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto native = carriers->get_type(result_type);
  if (!native || result.get_layout().get_size() != 1 ||
      LLVMGetTypeKind(LLVMTypeOf(receiver)) != LLVMStructTypeKind ||
      LLVMCountStructElementTypes(LLVMTypeOf(receiver)) < 2) {
    return call_fail_toolchain(
        body,
        "LLVM cannot read size from the completed contiguous receiver carrier."_view);
  }

  LLVMValueRef size =
      LLVMBuildExtractValue(native_body.get_builder(), receiver, 1, "size");
  if (!size || LLVMTypeOf(size) != *native ||
      !call_release_owned(native_body, *carriers, receiver_type, receiver)) {
    return False;
  }

  Core::Static::Vector<LLVMValueRef, 1> values = {{size}};
  native_body.mark_owned(result_type, size);
  return native_body.publish_values(result, values.get_view());
}

auto Llvm::Emission::Invocation::contiguous_is_empty(
    const Tetrodotoxin::Source::Pack& result,
    const Tetrodotoxin::Source::Type& result_type,
    const Tetrodotoxin::Source::Type& receiver_type,
    LLVMValueRef receiver) const -> Bool {
  Llvm::Module::Body& native_body = body;
  auto carriers = call_select_carriers(body);
  auto native =
      carriers ? carriers->get_type(result_type) : Core::Option<LLVMTypeRef>();
  if (!carriers || !native || result.get_layout().get_size() != 1 ||
      LLVMGetTypeKind(LLVMTypeOf(receiver)) != LLVMStructTypeKind ||
      LLVMCountStructElementTypes(LLVMTypeOf(receiver)) < 2) {
    return call_fail_toolchain(
        body,
        "LLVM cannot test the completed contiguous receiver carrier for emptiness."_view);
  }

  LLVMValueRef size =
      LLVMBuildExtractValue(native_body.get_builder(), receiver, 1, "size");
  LLVMValueRef zero = size ? LLVMConstNull(LLVMTypeOf(size)) : nullptr;
  LLVMValueRef empty =
      zero ? LLVMBuildICmp(
                 native_body.get_builder(), LLVMIntEQ, size, zero, "empty")
           : nullptr;
  if (!empty || LLVMTypeOf(empty) != *native ||
      !call_release_owned(native_body, *carriers, receiver_type, receiver)) {
    return False;
  }

  Core::Static::Vector<LLVMValueRef, 1> values = {{empty}};
  native_body.mark_owned(result_type, empty);
  return native_body.publish_values(result, values.get_view());
}

static auto object_element_size(
    Llvm::Module::Body& body,
    const Llvm::Module::Carriers& carriers,
    const Tetrodotoxin::Source::Type& receiver_type) -> Core::Option<Count> {
  auto element = carriers.get_element(receiver_type);
  auto native =
      element ? carriers.get_type(*element) : Core::Option<LLVMTypeRef>();
  if (!element || !native) {
    return {};
  }

  auto& module = *llvm::unwrap(&body.get_program().get_module());
  return module.getDataLayout()
      .getTypeAllocSize(llvm::unwrap(*native))
      .getFixedValue();
}

static auto object_capacity_value(
    Llvm::Module::Body& body,
    const Llvm::Module::Carriers& carriers,
    const Tetrodotoxin::Source::Type& receiver_type,
    LLVMValueRef receiver) -> Core::Option<LLVMValueRef> {
  auto element_size = object_element_size(body, carriers, receiver_type);
  if (!element_size || *element_size == 0) {
    return {};
  }

  auto& module = *llvm::unwrap(&body.get_program().get_module());
  llvm::LLVMContext& context = module.getContext();
  llvm::Type& pointer = *llvm::PointerType::getUnqual(context);
  llvm::Type& count = *llvm::Type::getInt64Ty(context);
  llvm::FunctionType& signature =
      *llvm::FunctionType::get(&count, {&pointer}, false);
  llvm::IRBuilder<>& builder =
      *reinterpret_cast<llvm::IRBuilder<>*>(body.get_builder());
  llvm::StringRef symbol(
      reinterpret_cast<const char*>(
          Perimortem::Abi::Core::object_capacity_symbol.get_data()),
      Perimortem::Abi::Core::object_capacity_symbol.get_size());
  llvm::Value& bytes = *builder.CreateCall(
      module.getOrInsertFunction(symbol, &signature), {llvm::unwrap(receiver)},
      "object.bytes");
  llvm::Value& divisor = *llvm::ConstantInt::get(&count, *element_size);
  return llvm::wrap(builder.CreateUDiv(&bytes, &divisor, "object.capacity"));
}

auto Llvm::Emission::Invocation::object_capacity(
    const Tetrodotoxin::Source::Pack& result,
    const Tetrodotoxin::Source::Type& result_type,
    const Tetrodotoxin::Source::Type& receiver_type,
    LLVMValueRef receiver) const -> Bool {
  auto carriers = call_select_carriers(body);
  auto native_result =
      carriers ? carriers->get_type(result_type) : Core::Option<LLVMTypeRef>();
  auto capacity =
      carriers ? object_capacity_value(body, *carriers, receiver_type, receiver)
               : Core::Option<LLVMValueRef>();
  if (!carriers || !native_result || !capacity ||
      LLVMTypeOf(*capacity) != *native_result ||
      !call_release_owned(body, *carriers, receiver_type, receiver)) {
    return call_fail_toolchain(
        body, "LLVM cannot read capacity from Object[T]."_view);
  }

  Core::Static::Vector<LLVMValueRef, 1> values = {{*capacity}};
  body.mark_owned(result_type, *capacity);
  return body.publish_values(result, values.get_view());
}

auto Llvm::Emission::Invocation::object_is_shared(
    const Tetrodotoxin::Source::Pack& result,
    const Tetrodotoxin::Source::Type& result_type,
    const Tetrodotoxin::Source::Type& receiver_type,
    const Tetrodotoxin::Source::Pack& receiver_source,
    LLVMValueRef receiver) const -> Bool {
  auto carriers = call_select_carriers(body);
  auto native_result =
      carriers ? carriers->get_type(result_type) : Core::Option<LLVMTypeRef>();
  auto native_receiver = carriers ? carriers->get_type(receiver_type)
                                  : Core::Option<LLVMTypeRef>();
  if (!carriers || !native_result || result.get_layout().get_size() != 1 ||
      !native_receiver || LLVMTypeOf(receiver) != *native_receiver) {
    return call_fail_toolchain(
        body, "LLVM cannot query Object[T] sharing state."_view);
  }

  auto& module = *llvm::unwrap(&body.get_program().get_module());
  llvm::LLVMContext& context = module.getContext();
  llvm::Type& pointer = *llvm::PointerType::getUnqual(context);
  llvm::Type& count = *llvm::Type::getInt64Ty(context);
  llvm::FunctionType& signature =
      *llvm::FunctionType::get(&count, {&pointer}, false);
  llvm::IRBuilder<>& builder =
      *reinterpret_cast<llvm::IRBuilder<>*>(body.get_builder());
  llvm::StringRef symbol(
      reinterpret_cast<const char*>(
          Perimortem::Abi::Core::object_reservations_symbol.get_data()),
      Perimortem::Abi::Core::object_reservations_symbol.get_size());
  llvm::Value& reservations = *builder.CreateCall(
      module.getOrInsertFunction(symbol, &signature), {llvm::unwrap(receiver)},
      "object.reservations");

  // Loading a stored Object creates one owned evaluation value in addition to
  // its storage owner. A computed Object has only that evaluation owner. The
  // query excludes exactly those local reservations from its public result.
  U64 local_reservations = body.find_target_address(receiver_source) ? 2 : 1;
  llvm::Value& local = *llvm::ConstantInt::get(&count, local_reservations);
  LLVMValueRef shared =
      llvm::wrap(builder.CreateICmpUGT(&reservations, &local, "object.shared"));
  if (!shared || LLVMTypeOf(shared) != *native_result ||
      !call_release_owned(body, *carriers, receiver_type, receiver)) {
    return False;
  }

  Core::Static::Vector<LLVMValueRef, 1> values = {{shared}};
  body.mark_owned(result_type, shared);
  return body.publish_values(result, values.get_view());
}

auto Llvm::Emission::Invocation::object_clone(
    const Tetrodotoxin::Source::Pack& result,
    const Tetrodotoxin::Source::Type& receiver_type,
    const Tetrodotoxin::Source::Pack& receiver_source,
    LLVMValueRef receiver) const -> Bool {
  auto carriers = call_select_carriers(body);
  auto target = body.find_target_address(receiver_source);
  auto element_size = carriers
                          ? object_element_size(body, *carriers, receiver_type)
                          : Core::Option<Count>();
  auto descriptor = carriers ? carriers->get_object_descriptor(
                                   body.get_program(), receiver_type)
                             : Core::Option<LLVMValueRef>();
  if (!carriers || !target || &target->get_type() != &receiver_type ||
      !element_size || !descriptor || !result.get_layout().is_empty()) {
    return call_fail_toolchain(
        body, "LLVM cannot clone the selected Object[T] buffer."_view);
  }

  if (body.take_owned(receiver) &&
      !carriers->release(body, receiver_type, receiver)) {
    return False;
  }

  auto& module = *llvm::unwrap(&body.get_program().get_module());
  llvm::LLVMContext& context = module.getContext();
  llvm::Type& pointer = *llvm::PointerType::getUnqual(context);
  llvm::Type& count = *llvm::Type::getInt64Ty(context);
  llvm::FunctionType& signature =
      *llvm::FunctionType::get(&pointer, {&pointer, &pointer, &count}, false);
  llvm::IRBuilder<>& builder =
      *reinterpret_cast<llvm::IRBuilder<>*>(body.get_builder());
  llvm::StringRef symbol(
      reinterpret_cast<const char*>(
          Perimortem::Abi::Core::object_clone_symbol.get_data()),
      Perimortem::Abi::Core::object_clone_symbol.get_size());
  llvm::Value& size = *llvm::ConstantInt::get(&count, *element_size);
  llvm::Value& cloned = *builder.CreateCall(
      module.getOrInsertFunction(symbol, &signature),
      {llvm::unwrap(receiver), llvm::unwrap(*descriptor), &size},
      "object.clone");
  builder.CreateStore(&cloned, llvm::unwrap(target->get_address()));
  return call_publish_empty(body, result);
}

auto Llvm::Emission::Invocation::object_view(
    const Tetrodotoxin::Source::Pack& result,
    const Tetrodotoxin::Source::Type& result_type,
    const Tetrodotoxin::Source::Type& receiver_type,
    LLVMValueRef receiver) const -> Bool {
  auto carriers = call_select_carriers(body);
  auto native_result =
      carriers ? carriers->get_type(result_type) : Core::Option<LLVMTypeRef>();
  auto capacity =
      carriers ? object_capacity_value(body, *carriers, receiver_type, receiver)
               : Core::Option<LLVMValueRef>();
  if (!native_result || !capacity ||
      LLVMGetTypeKind(*native_result) != LLVMStructTypeKind ||
      LLVMCountStructElementTypes(*native_result) != 2) {
    return call_fail_toolchain(
        body, "LLVM cannot borrow Object[T] storage."_view);
  }

  LLVMValueRef view = LLVMGetUndef(*native_result);
  view = LLVMBuildInsertValue(
      body.get_builder(), view, receiver, 0, "object.data");
  view = LLVMBuildInsertValue(
      body.get_builder(), view, *capacity, 1, "object.size");
  Core::Static::Vector<LLVMValueRef, 1> values = {{view}};
  return view && body.publish_values(result, values.get_view());
}

static auto reserve_object(
    Llvm::Module::Body& body,
    const Llvm::Module::Carriers& carriers,
    const Tetrodotoxin::Source::Type& receiver_type,
    const Tetrodotoxin::Source::Pack& receiver_source,
    LLVMValueRef receiver,
    LLVMValueRef count,
    const Tetrodotoxin::Library::Language::Model::Pack& element_default)
    -> Core::Option<LLVMValueRef> {
  auto target = body.find_target_address(receiver_source);
  auto element = carriers.get_element(receiver_type);
  auto native_element =
      element ? carriers.get_type(*element) : Core::Option<LLVMTypeRef>();
  auto element_size = object_element_size(body, carriers, receiver_type);
  auto defaults = body.find_values(element_default);
  auto default_value =
      element && defaults
          ? carriers.fit_and_assemble(
                body, *element, element_default, defaults->get_view())
          : Core::Option<LLVMValueRef>();
  auto descriptor = body.get_program().get_carriers().get_object_descriptor(
      body.get_program(), receiver_type);
  if (!target || &target->get_type() != &receiver_type || !element ||
      !native_element || !element_size || !default_value || !descriptor) {
    return {};
  }

  LLVMValueRef default_address =
      body.create_entry_alloca(*native_element, "object.default"_view);
  LLVMBuildStore(body.get_builder(), *default_value, default_address);

  if (body.take_owned(receiver) &&
      !carriers.release(body, receiver_type, receiver)) {
    return {};
  }

  auto& module = *llvm::unwrap(&body.get_program().get_module());
  llvm::LLVMContext& context = module.getContext();
  llvm::Type& pointer = *llvm::PointerType::getUnqual(context);
  llvm::Type& native_count = *llvm::Type::getInt64Ty(context);
  llvm::FunctionType& signature = *llvm::FunctionType::get(
      &pointer, {&pointer, &pointer, &native_count, &native_count, &pointer},
      false);
  llvm::IRBuilder<>& builder =
      *reinterpret_cast<llvm::IRBuilder<>*>(body.get_builder());
  llvm::StringRef symbol(
      reinterpret_cast<const char*>(
          Perimortem::Abi::Core::object_reserve_symbol.get_data()),
      Perimortem::Abi::Core::object_reserve_symbol.get_size());
  llvm::Value& size = *llvm::ConstantInt::get(&native_count, *element_size);
  llvm::Value& selected = *builder.CreateCall(
      module.getOrInsertFunction(symbol, &signature),
      {llvm::unwrap(receiver), llvm::unwrap(*descriptor), llvm::unwrap(count),
       &size, llvm::unwrap(default_address)},
      "object.reserve");
  builder.CreateStore(&selected, llvm::unwrap(target->get_address()));
  return llvm::wrap(&selected);
}

static auto publish_object_access(
    Llvm::Module::Body& body,
    const Llvm::Module::Carriers& carriers,
    const Tetrodotoxin::Source::Pack& result,
    const Tetrodotoxin::Source::Type& result_type,
    const Tetrodotoxin::Source::Type& receiver_type,
    LLVMValueRef data) -> Bool {
  auto native_result = carriers.get_type(result_type);
  auto capacity = object_capacity_value(body, carriers, receiver_type, data);
  if (!native_result || !capacity ||
      LLVMGetTypeKind(*native_result) != LLVMStructTypeKind ||
      LLVMCountStructElementTypes(*native_result) != 2) {
    return False;
  }

  LLVMValueRef access = LLVMGetUndef(*native_result);
  access =
      LLVMBuildInsertValue(body.get_builder(), access, data, 0, "object.data");
  access = LLVMBuildInsertValue(
      body.get_builder(), access, *capacity, 1, "object.size");
  Core::Static::Vector<LLVMValueRef, 1> values = {{access}};
  return access && body.publish_values(result, values.get_view());
}

auto Llvm::Emission::Invocation::object_access(
    const Tetrodotoxin::Source::Pack& result,
    const Tetrodotoxin::Source::Type& result_type,
    const Tetrodotoxin::Source::Type& receiver_type,
    const Tetrodotoxin::Source::Pack& receiver_source,
    LLVMValueRef receiver,
    const Library::Language::Model::Pack& element_default) const -> Bool {
  auto carriers = call_select_carriers(body);
  auto count =
      carriers ? object_capacity_value(body, *carriers, receiver_type, receiver)
               : Core::Option<LLVMValueRef>();
  auto selected = carriers && count
                      ? reserve_object(
                            body, *carriers, receiver_type, receiver_source,
                            receiver, *count, element_default)
                      : Core::Option<LLVMValueRef>();
  return carriers && selected &&
         publish_object_access(
             body, *carriers, result, result_type, receiver_type, *selected);
}

auto Llvm::Emission::Invocation::object_reserve(
    const Tetrodotoxin::Source::Pack& result,
    const Tetrodotoxin::Source::Type& result_type,
    const Tetrodotoxin::Source::Type& receiver_type,
    const Tetrodotoxin::Source::Pack& receiver_source,
    LLVMValueRef receiver,
    LLVMValueRef count,
    const Library::Language::Model::Pack& element_default) const -> Bool {
  auto carriers = call_select_carriers(body);
  auto selected = carriers
                      ? reserve_object(
                            body, *carriers, receiver_type, receiver_source,
                            receiver, count, element_default)
                      : Core::Option<LLVMValueRef>();
  return carriers && selected &&
         publish_object_access(
             body, *carriers, result, result_type, receiver_type, *selected);
}

// A borrow needs storage that outlives receiver evaluation. An Addressable
// supplies that storage directly. A computed Fixed moves into one Body owned
// slot so the borrowed pointer remains valid through enclosing scope cleanup.
static auto create_fixed_borrow(
    Llvm::Module::Body& body,
    const Tetrodotoxin::Source::Pack& result,
    const Tetrodotoxin::Source::Type& result_type,
    const Tetrodotoxin::Source::Type& receiver_type,
    const Tetrodotoxin::Source::Pack& receiver_source,
    LLVMValueRef receiver) -> Bool {
  auto& native_body = body;
  auto carriers = call_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto storage = native_body.find_target_address(receiver_source);
  const Tetrodotoxin::Source::Type& fixed_type = receiver_type;
  auto fixed_native = carriers->get_type(fixed_type);
  auto extent = carriers->get_extent(fixed_type);
  auto view_native = carriers->get_type(result_type);
  if ((storage && &storage->get_type() != &fixed_type) || !fixed_native ||
      !extent || !view_native || result.get_layout().get_size() != 1 ||
      LLVMGetTypeKind(*fixed_native) != LLVMArrayTypeKind ||
      LLVMGetArrayLength2(*fixed_native) != *extent ||
      LLVMGetTypeKind(*view_native) != LLVMStructTypeKind ||
      LLVMCountStructElementTypes(*view_native) != 2) {
    return call_fail_toolchain(
        body,
        "LLVM cannot borrow contiguous data from the exact Fixed storage carrier."_view);
  }

  Core::Option<LLVMValueRef> address;
  if (storage) {
    address = storage->get_address();
  } else {
    LLVMValueRef allocated =
        native_body.create_entry_alloca(*fixed_native, "fixed.borrow"_view);
    LLVMBuildStore(native_body.get_builder(), receiver, allocated);
    if (!native_body.acquire(fixed_type, receiver) ||
        !native_body.register_storage(fixed_type, allocated) ||
        !native_body.publish_target_address(
            receiver_source, fixed_type, allocated)) {
      return False;
    }

    address = allocated;
  }

  LLVMContextRef context = LLVMGetTypeContext(*fixed_native);
  LLVMValueRef zero = LLVMConstInt(LLVMInt64TypeInContext(context), 0, 0);
  Core::Static::Vector<LLVMValueRef, 2> indices = {{zero, zero}};
  LLVMValueRef data = LLVMBuildInBoundsGEP2(
      native_body.get_builder(), *fixed_native, *address, indices.get_data(),
      U32(indices.get_size()), "fixed.data");
  if (!data) {
    return call_fail_toolchain(
        body, "LLVM could not select the first element of Fixed storage."_view);
  }

  LLVMValueRef count =
      LLVMConstInt(LLVMInt64TypeInContext(context), U64(*extent), 0);
  LLVMValueRef view = LLVMGetUndef(*view_native);
  view = LLVMBuildInsertValue(
      native_body.get_builder(), view, data, 0, "fixed.data");
  view = LLVMBuildInsertValue(
      native_body.get_builder(), view, count, 1, "fixed.size");
  if (!view) {
    return call_fail_toolchain(
        body,
        "LLVM could not construct a borrow over the selected Fixed storage."_view);
  }

  if (storage &&
      !call_release_owned(native_body, *carriers, fixed_type, receiver)) {
    return False;
  }

  Core::Static::Vector<LLVMValueRef, 1> values = {{view}};
  native_body.mark_owned(result_type, view);
  return native_body.publish_values(result, values.get_view());
}

auto Llvm::Emission::Invocation::borrow_fixed(
    const Tetrodotoxin::Source::Pack& result,
    const Tetrodotoxin::Source::Type& result_type,
    const Tetrodotoxin::Source::Type& receiver_type,
    const Tetrodotoxin::Source::Pack& receiver_source,
    LLVMValueRef receiver) const -> Bool {
  return create_fixed_borrow(
      body, result, result_type, receiver_type, receiver_source, receiver);
}

auto Llvm::Emission::Invocation::slice_view(
    const Tetrodotoxin::Source::Pack& result,
    const Tetrodotoxin::Source::Type& result_type,
    const Tetrodotoxin::Source::Type& receiver_type,
    LLVMValueRef receiver,
    LLVMValueRef start,
    LLVMValueRef count) const -> Bool {
  Llvm::Module::Body& native_body = body;
  auto carriers = call_select_carriers(body);
  if (!carriers) {
    return False;
  }

  auto element = carriers->get_element(receiver_type);
  auto native_element =
      element ? carriers->get_type(*element) : Core::Option<LLVMTypeRef>();
  auto native_result = carriers->get_type(result_type);
  Count receiver_fields =
      LLVMGetTypeKind(LLVMTypeOf(receiver)) == LLVMStructTypeKind
          ? LLVMCountStructElementTypes(LLVMTypeOf(receiver))
          : 0;
  if (!element || !native_element || !native_result ||
      result.get_layout().get_size() != 1 || receiver_fields != 2 ||
      LLVMGetTypeKind(*native_result) != LLVMStructTypeKind ||
      LLVMCountStructElementTypes(*native_result) != 2) {
    return call_fail_toolchain(
        body,
        "LLVM cannot slice the completed contiguous receiver carrier."_view);
  }

  LLVMBuilderRef builder = native_body.get_builder();
  LLVMValueRef data = LLVMBuildExtractValue(builder, receiver, 0, "slice.data");
  LLVMValueRef length =
      LLVMBuildExtractValue(builder, receiver, 1, "slice.length");
  if (!data || !length || LLVMTypeOf(start) != LLVMTypeOf(length) ||
      LLVMTypeOf(count) != LLVMTypeOf(length)) {
    return call_fail_toolchain(
        body, "LLVM cannot align slice indices with the receiver size."_view);
  }

  // Perimortem clipping keeps the available suffix and uses a canonical empty
  // View when the requested start is unavailable.
  LLVMValueRef present =
      LLVMBuildICmp(builder, LLVMIntULT, start, length, "slice.present");
  LLVMValueRef remaining =
      LLVMBuildSub(builder, length, start, "slice.remaining");
  LLVMValueRef short_request =
      LLVMBuildICmp(builder, LLVMIntULT, count, remaining, "slice.short");
  LLVMValueRef selected_count =
      LLVMBuildSelect(builder, short_request, count, remaining, "slice.count");
  LLVMValueRef zero = LLVMConstNull(LLVMTypeOf(length));
  LLVMValueRef size =
      LLVMBuildSelect(builder, present, selected_count, zero, "slice.size");
  LLVMValueRef offset = start;
  LLVMValueRef selected_data = LLVMBuildGEP2(
      builder, *native_element, data, &offset, 1, "slice.selected.data");
  LLVMValueRef empty_data = LLVMConstNull(LLVMTypeOf(data));
  LLVMValueRef view_data = LLVMBuildSelect(
      builder, present, selected_data, empty_data, "slice.view.data");

  LLVMValueRef view = LLVMGetUndef(*native_result);
  view = LLVMBuildInsertValue(builder, view, view_data, 0, "slice.view.data");
  view = LLVMBuildInsertValue(builder, view, size, 1, "slice.view.size");
  if (!view) {
    return False;
  }

  if (!call_release_owned(native_body, *carriers, receiver_type, receiver)) {
    return False;
  }

  Core::Static::Vector<LLVMValueRef, 1> values = {{view}};
  native_body.mark_owned(result_type, view);
  return native_body.publish_values(result, values.get_view());
}

// Comparison uses the operand Type supplied by the semantic owner to preserve
// signedness that cannot be recovered from an LLVM integer Type.

auto Llvm::Emission::Invocation::construct(
    const Library::Language::Model::Pack& result,
    const Tetrodotoxin::Source::Type& type,
    const Library::Language::Model::Pack& values) const -> Bool {
  Llvm::Module::Body& native_body = body;
  Llvm::Module::Program& native_program = body.get_program();
  const Llvm::Module::Carriers& carriers = native_program.get_carriers();

  auto elements = native_body.find_values(values);
  if (!elements) {
    return native_program.fail_toolchain(
        "LLVM cannot construct a value before its completed initializer Pack is lowered."_view);
  }

  auto value = carriers.construct(native_body, type, elements->get_view());
  if (!value) {
    return False;
  }

  Core::Static::Vector<LLVMValueRef, 1> native = {{*value}};
  return native_body.publish_values(result, native.get_view());
}

auto Llvm::Emission::Invocation::construct_provider(
    const Library::Language::Model::Pack& result,
    const Tetrodotoxin::Source::Type& type,
    const Library::Language::Model::Pack& arguments,
    Core::View::Vector<Tetrodotoxin::Source::Reference<const Tetrodotoxin::Source::Addressable>>
        parameters) const -> Bool {
  auto& program = body.get_program();
  const auto& functions = program.get_functions();
  return functions.reserve_construction(program, type, False, parameters) &&
         functions.complete_construction(program, type) &&
         functions.call_construction(body, result, type, arguments);
}

// Control operations return transient block handles to their semantic owners.
// Body retains only loop targets needed by nested break and continue owners.
