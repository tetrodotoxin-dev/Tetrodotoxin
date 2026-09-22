// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/terminal/spirv/module/body.hpp"

#include "tetrodotoxin/library/language/access/address.hpp"
#include "tetrodotoxin/library/language/access/call.hpp"
#include "tetrodotoxin/library/language/access/swizzle.hpp"
#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/diagnostics.hpp"
#include "tetrodotoxin/library/language/expressions/conversion.hpp"
#include "tetrodotoxin/library/language/expressions/identifier.hpp"
#include "tetrodotoxin/library/language/expressions/initializer.hpp"
#include "tetrodotoxin/library/language/flow/local.hpp"
#include "tetrodotoxin/library/language/flow/return.hpp"
#include "tetrodotoxin/library/language/function.hpp"
#include "tetrodotoxin/library/language/model/types/real.hpp"
#include "tetrodotoxin/library/language/model/types/unsigned.hpp"
#include "tetrodotoxin/library/language/operation.hpp"
#include "tetrodotoxin/library/language/operations/add.hpp"
#include "tetrodotoxin/library/language/operations/divide.hpp"
#include "tetrodotoxin/library/language/operations/modulo.hpp"
#include "tetrodotoxin/library/language/operations/multiply.hpp"
#include "tetrodotoxin/library/language/operations/subtract.hpp"
#include "tetrodotoxin/library/language/types/structure.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Terminal::Spirv;

static auto anchor_of(const Abstract& semantic) -> Tetrodotoxin::Source::Lexical::Anchor {
  auto expression = semantic.select<Library::Language::Expression>();
  if (expression && expression->get_anchor()) {
    return *expression->get_anchor();
  }
  auto local = semantic.select<Library::Language::Flow::Local>();
  if (local) {
    return local->get_anchor();
  }
  auto returned = semantic.select<Library::Language::Flow::Return>();
  if (returned) {
    return returned->get_anchor();
  }
  auto function = semantic.select<Library::Language::Function>();
  return function ? function->get_anchor()
                  : Tetrodotoxin::Source::Lexical::Anchor::create(Tetrodotoxin::Source::Lexical::Span());
}

static auto constant_conversion(
    const Library::Language::Expressions::Conversion& conversion)
    -> Core::Option<const Library::Language::Model::Pack&> {
  auto folded = conversion.get_folded();
  auto producer = folded && folded->get_layout().get_size() == 1
                      ? folded->get_layout().get_abstract(0)
                      : Core::Option<const Abstract&>();
  auto constant = producer ? producer->select<Library::Language::Constant>()
                           : Core::Option<const Library::Language::Constant&>();
  return constant ? folded
                  : Core::Option<const Library::Language::Model::Pack&>();
}

static auto select_scalar_pack(
    const Library::Language::Model::Pack& pack,
    Count index) -> Core::Option<const Library::Language::Model::Pack&> {
  auto swizzle = pack.select_identity<Library::Language::Access::Swizzle>();
  if (swizzle) {
    const auto projections = swizzle->get_projections();
    if (!projections.is_empty()) {
      BAIL_IF(index >= projections.get_size());
      return Library::Language::Model::Pack::from(
          projections.get_data()[index].get());
    }
    const auto selections = swizzle->get_selections();
    BAIL_IF(index >= selections.get_size());
    return select_scalar_pack(swizzle->get_receiver(), selections[index]);
  }
  if (pack.get_identity()) {
    return index == 0 && pack.get_layout().get_size() == 1
               ? Core::Option<const Library::Language::Model::Pack&>(pack)
               : Core::Option<const Library::Language::Model::Pack&>();
  }

  Count offset = 0;
  for (const Tetrodotoxin::Source::PackReference<Library::Language::Model::Pack>& entry :
       pack.get_entries()) {
    Count size = entry.get().get_layout().get_size();
    if (index < offset + size) {
      return select_scalar_pack(entry.get(), index - offset);
    }
    offset += size;
  }
  return {};
}

static auto is_sample_call(const Library::Language::Access::Call& call)
    -> Bool {
  auto callable = call.get_callable();
  auto function = callable ? callable->select<Library::Language::Function>()
                           : Core::Option<const Library::Language::Function&>();
  BAIL_IF(!function);
  for (const Tetrodotoxin::Language::Attribute& attribute :
       function->get_definition().get_attributes()) {
    const Core::View::Bytes* value =
        attribute.get_value().find<Core::View::Bytes>();
    if (attribute.get_key() == "intrinsic"_view && value &&
        *value == "sample_2d"_view) {
      return True;
    }
  }
  return False;
}

auto Module::Body::reject(const Abstract& semantic, Core::View::Bytes message)
    const -> Bool {
  Tetrodotoxin::Source::Lexical::Errors::Report report(
      request.get_errors(), request.get_source_path(),
      request.get_source_text(), anchor_of(semantic));
  report << message;
  report.get_hint()
      << "Use a Library operation currently admitted by the SPIR V target."_view;
  return False;
}

auto Module::Body::prepare(const Interface::Stage& stage) -> Bool {
  // Preparation proves the complete supported graph before section ordered
  // emission begins. Types and Constants can then appear before Function words
  // without retaining a second executable model.
  const auto body = stage.function.get().get_body();
  BAIL_IF(!body);
  for (const Library::Language::Statement& statement : body->get_statements()) {
    const Abstract& root = statement.get_root();
    auto local = root.select<Library::Language::Flow::Local>();
    if (local) {
      auto type = local->get_linked_type();
      auto initializer = local->get_initializer();
      BAIL_IF(
          !type || !initializer || !types.collect(*type) ||
          !prepare_pack(*initializer));
      continue;
    }
    auto returned = root.select<Library::Language::Flow::Return>();
    if (returned) {
      BAIL_IF(!prepare_pack(returned->get_pack()));
      continue;
    }
    auto expression = root.select<Library::Language::Expression>();
    if (expression) {
      BAIL_IF(!prepare_expression(*expression));
      continue;
    }
    return reject(
        root, "This Library Statement has no SPIR V lowering yet."_view);
  }
  return True;
}

auto Module::Body::prepare_pack(const Library::Language::Model::Pack& pack)
    -> Bool {
  auto constant = pack.select_identity<Library::Language::Constant>();
  if (constant) {
    return constants.collect(*constant);
  }
  auto expression = pack.select_identity<Library::Language::Expression>();
  if (expression) {
    return prepare_expression(*expression);
  }
  for (const Tetrodotoxin::Source::PackReference<Library::Language::Model::Pack>& entry :
       pack.get_entries()) {
    BAIL_IF(!prepare_pack(entry.get()));
  }
  return True;
}

auto Module::Body::prepare_expression(
    const Library::Language::Expression& expression) -> Bool {
  // A Swizzle preserves its selected producers. Prepare those scalar values
  // without inventing a vector Type for positional flow.
  auto swizzle = expression.select<Library::Language::Access::Swizzle>();
  if (swizzle) {
    for (Count index = 0; index < swizzle->get_layout().get_size(); index++) {
      auto selected = select_scalar_pack(*swizzle, index);
      BAIL_IF(!selected || !prepare_pack(*selected));
    }
    return True;
  }
  auto type = Types::select(expression.get_type());
  BAIL_IF(
      type && !interface.is_resource(expression.get_result()) &&
      !types.collect(*type));

  auto constant = expression.select<Library::Language::Constant>();
  if (constant) {
    return constants.collect(*constant);
  }
  if (expression.is<Library::Language::Expressions::Identifier>()) {
    return True;
  }
  auto conversion =
      expression.select<Library::Language::Expressions::Conversion>();
  if (conversion) {
    auto folded = constant_conversion(*conversion);
    if (folded) {
      return prepare_pack(*folded);
    }
    const auto& source = conversion->get_source();
    auto source_type = Types::select(source.get_value_type(0));
    auto target_real =
        type ? type->select<Library::Language::Model::Types::Real>()
             : Core::Option<const Library::Language::Model::Types::Real&>();
    auto source_real =
        source_type
            ? source_type->select<Library::Language::Model::Types::Real>()
            : Core::Option<const Library::Language::Model::Types::Real&>();
    auto source_unsigned =
        source_type
            ? source_type->select<Library::Language::Model::Types::Unsigned>()
            : Core::Option<const Library::Language::Model::Types::Unsigned&>();
    BAIL_IF(
        !source_type || !target_real || (!source_real && !source_unsigned) ||
        !prepare_pack(source));
    return True;
  }
  auto address = expression.select<Library::Language::Access::Address>();
  if (address) {
    return prepare_pack(address->get_receiver());
  }
  auto call = expression.select<Library::Language::Access::Call>();
  if (call) {
    BAIL_IF(
        !is_sample_call(*call) || !prepare_pack(call->get_receiver()) ||
        !prepare_pack(call->get_arguments()));
    return True;
  }
  auto operation = expression.select<Library::Language::Operation>();
  if (operation) {
    if (!type || !type->is<Library::Language::Model::Types::Real>()) {
      return reject(
          expression,
          "This arithmetic operation has no floating point SPIR V form."_view);
    }
    for (const Tetrodotoxin::Source::PackReference<Library::Language::Model::Pack>&
             input : operation->get_inputs()) {
      BAIL_IF(!prepare_pack(input.get()));
    }
    return True;
  }
  auto initializer =
      expression.select<Library::Language::Expressions::Initializer>();
  if (initializer) {
    auto completed = initializer->get_completed_values();
    return completed ? prepare_pack(*completed)
                     : prepare_pack(initializer->get_arguments());
  }
  return reject(
      expression, "This Library Expression has no SPIR V lowering yet."_view);
}

auto Module::Body::find_value(const Abstract& semantic) const
    -> Core::Option<Value> {
  for (const Value& value : values.get_view()) {
    if (&value.semantic.get() == &semantic) {
      return value;
    }
  }
  return {};
}

auto Module::Body::retain_value(Value value) -> Bool {
  auto retained = find_value(value.semantic.get());
  BAIL_IF(
      retained &&
      (retained->id != value.id || &retained->type.get() != &value.type.get()));
  if (!retained) {
    values.insert(value);
  }
  return True;
}

auto Module::Body::select_source(
    const Library::Language::Model::Pack& pack,
    const Tetrodotoxin::Source::Layout& target,
    Count target_index) -> Core::Option<Count> {
  auto target_name = target.get_name(target_index);
  if (!target_name) {
    return target_index < pack.get_layout().get_size()
               ? Core::Option<Count>(target_index)
               : Core::Option<Count>();
  }

  Bool named_source = False;
  Core::Option<Count> selected;
  for (Count index = 0; index < pack.get_layout().get_size(); index++) {
    auto source_name = pack.get_layout().get_name(index);
    named_source |= Bool(source_name);
    if (source_name && *source_name == *target_name) {
      BAIL_IF(selected);
      selected = index;
    }
  }
  if (named_source) {
    return selected;
  }
  return target_index < pack.get_layout().get_size()
             ? Core::Option<Count>(target_index)
             : Core::Option<Count>();
}

auto Module::Body::lower_pack(
    const Library::Language::Model::Pack& pack,
    const Library::Language::Model::Type& expected,
    Assembler::SpirV& assembler) -> Core::Option<Value> {
  // Scalar producers keep their own result id. A Structure is assembled from
  // the real producers selected by Pack fitting in target field order.
  auto constant = pack.select_identity<Library::Language::Constant>();
  if (constant) {
    auto id = constants.get_id(*constant);
    auto type = Types::select(constant->get_type());
    BAIL_IF(!id || !type || &*type != &expected);
    return Value(*constant, expected, *id);
  }

  auto expression = pack.select_identity<Library::Language::Expression>();
  if (expression && !expression->is<Library::Language::Access::Swizzle>()) {
    auto lowered = lower_expression(*expression, assembler);
    auto source_id =
        lowered ? types.get_id(lowered->type.get()) : Core::Option<U32>();
    auto expected_id = types.get_id(expected);
    BAIL_IF(
        !lowered || !source_id || !expected_id || *source_id != *expected_id);
    return *lowered;
  }

  auto structure = expected.select<Library::Language::Types::Structure>();
  if (!structure) {
    BAIL_IF(pack.get_layout().get_size() != 1);
    auto child = select_scalar_pack(pack, 0);
    BAIL_IF(!child);
    return lower_pack(*child, expected, assembler);
  }

  const Tetrodotoxin::Source::Layout& target = structure->get_layout();
  BAIL_IF(!pack.fits(expected));
  Memory::Dynamic::Vector<U32> members;
  for (Count index = 0; index < target.get_size(); index++) {
    auto source_index = select_source(pack, target, index);
    auto member_semantic = target.get_abstract(index);
    auto member_type =
        member_semantic ? Types::select(*member_semantic)
                        : Core::Option<const Library::Language::Model::Type&>();
    auto child = source_index
                     ? select_scalar_pack(pack, *source_index)
                     : Core::Option<const Library::Language::Model::Pack&>();
    BAIL_IF(!member_type || !child);
    auto lowered = lower_pack(*child, *member_type, assembler);
    BAIL_IF(!lowered);
    members.insert(lowered->id);
  }
  auto type_id = types.get_id(expected);
  BAIL_IF(!type_id);
  U32 id = ids.take();
  assembler.composite_construct(*type_id, id, members.get_view());
  return Value(expected, expected, id);
}

auto Module::Body::lower_expression(
    const Library::Language::Expression& expression,
    Assembler::SpirV& assembler) -> Core::Option<Value> {
  // Reusing a retained value preserves Library DAG sharing while every fresh
  // operation result receives one module local id.
  auto cached = find_value(expression);
  if (cached) {
    return *cached;
  }
  auto type = Types::select(expression.get_type());
  BAIL_IF(!type);

  auto constant = expression.select<Library::Language::Constant>();
  if (constant) {
    auto id = constants.get_id(*constant);
    BAIL_IF(!id);
    Value value(expression, *type, *id);
    BAIL_IF(!retain_value(value));
    return value;
  }
  auto identifier =
      expression.select<Library::Language::Expressions::Identifier>();
  if (identifier) {
    auto selected = find_value(identifier->get_result());
    BAIL_IF(!selected || &selected->type.get() != &*type);
    Value value(expression, *type, selected->id);
    BAIL_IF(!retain_value(value));
    return value;
  }
  auto conversion =
      expression.select<Library::Language::Expressions::Conversion>();
  if (conversion) {
    auto folded = constant_conversion(*conversion);
    if (folded) {
      auto lowered = lower_pack(*folded, *type, assembler);
      BAIL_IF(!lowered);
      Value value(expression, *type, lowered->id);
      BAIL_IF(!retain_value(value));
      return value;
    }
    const auto& source = conversion->get_source();
    BAIL_IF(source.get_layout().get_size() != 1);
    auto source_type = Types::select(source.get_value_type(0));
    auto target_id = types.get_id(*type);
    auto lowered = source_type ? lower_pack(source, *source_type, assembler)
                               : Core::Option<Value>();
    auto source_real =
        source_type
            ? source_type->select<Library::Language::Model::Types::Real>()
            : Core::Option<const Library::Language::Model::Types::Real&>();
    auto source_unsigned =
        source_type
            ? source_type->select<Library::Language::Model::Types::Unsigned>()
            : Core::Option<const Library::Language::Model::Types::Unsigned&>();
    auto target_real = type->select<Library::Language::Model::Types::Real>();
    BAIL_IF(
        !lowered || !target_id || (!source_real && !source_unsigned) ||
        !target_real);
    U32 id = ids.take();
    if (source_real) {
      assembler.fconvert(*target_id, id, lowered->id);
    } else {
      assembler.convert_u_to_f(*target_id, id, lowered->id);
    }
    Value value(expression, *type, id);
    BAIL_IF(!retain_value(value));
    return value;
  }
  auto call = expression.select<Library::Language::Access::Call>();
  if (call) {
    BAIL_IF(!is_sample_call(*call));
    auto callable = call->get_callable();
    BAIL_IF(!callable || callable->get_parameters().get_size() != 2);
    const Tetrodotoxin::Source::Layout& parameters = callable->get_parameters();
    auto coordinate_semantic = parameters.get_abstract(1);
    auto coordinate_type =
        coordinate_semantic
            ? Types::select(*coordinate_semantic)
            : Core::Option<const Library::Language::Model::Type&>();
    auto coordinate_pack = select_scalar_pack(call->get_arguments(), 0);
    auto sampled_type = Types::select(call->get_receiver().get_type());
    auto sampled_image =
        sampled_type
            ? lower_pack(call->get_receiver(), *sampled_type, assembler)
            : Core::Option<Value>();
    auto coordinate =
        coordinate_type && coordinate_pack
            ? lower_pack(*coordinate_pack, *coordinate_type, assembler)
            : Core::Option<Value>();
    auto result_type = Types::select(call->get_type());
    auto result_id =
        result_type ? types.get_id(*result_type) : Core::Option<U32>();
    BAIL_IF(
        !sampled_image || !coordinate || !result_type || !result_id ||
        call->get_arguments().get_layout().get_size() != 1);
    U32 id = ids.take();
    assembler.image_sample_implicit_lod(
        *result_id, id, sampled_image->id, coordinate->id);
    Value value(expression, *result_type, id);
    BAIL_IF(!retain_value(value));
    return value;
  }
  auto address = expression.select<Library::Language::Access::Address>();
  if (address) {
    auto receiver_type = Types::select(address->get_receiver().get_type());
    auto receiver =
        receiver_type
            ? lower_pack(address->get_receiver(), *receiver_type, assembler)
            : Core::Option<Value>();
    BAIL_IF(!receiver);
    const Tetrodotoxin::Source::Layout& layout = receiver->type.get().get_layout();
    Core::Option<Count> member;
    for (Count index = 0; index < layout.get_size(); index++) {
      auto semantic = layout.get_abstract(index);
      if (semantic && &*semantic == &address->get_result()) {
        BAIL_IF(member);
        member = index;
      }
    }
    auto type_id = types.get_id(*type);
    BAIL_IF(!member || !type_id);
    U32 index = U32(*member);
    U32 id = ids.take();
    assembler.composite_extract(
        *type_id, id, receiver->id, Core::View::Vector<U32>(&index, 1));
    Value value(expression, *type, id);
    BAIL_IF(!retain_value(value));
    return value;
  }
  auto operation = expression.select<Library::Language::Operation>();
  if (operation) {
    BAIL_IF(!type->is<Library::Language::Model::Types::Real>());
    auto inputs = operation->get_inputs();
    BAIL_IF(inputs.get_size() != 2);
    auto left = lower_pack(inputs.get_data()[0].get(), *type, assembler);
    auto right = lower_pack(inputs.get_data()[1].get(), *type, assembler);
    auto type_id = types.get_id(*type);
    BAIL_IF(!left || !right || !type_id);
    U32 id = ids.take();
    if (operation->is<Library::Language::Operations::Add>()) {
      assembler.fadd(*type_id, id, left->id, right->id);
    } else if (operation->is<Library::Language::Operations::Subtract>()) {
      assembler.fsub(*type_id, id, left->id, right->id);
    } else if (operation->is<Library::Language::Operations::Multiply>()) {
      assembler.fmul(*type_id, id, left->id, right->id);
    } else if (operation->is<Library::Language::Operations::Divide>()) {
      assembler.fdiv(*type_id, id, left->id, right->id);
    } else if (operation->is<Library::Language::Operations::Modulo>()) {
      assembler.frem(*type_id, id, left->id, right->id);
    } else {
      reject(
          expression,
          "This Library Operation has no SPIR V instruction yet."_view);
      return {};
    }
    Value value(expression, *type, id);
    BAIL_IF(!retain_value(value));
    return value;
  }
  auto initializer =
      expression.select<Library::Language::Expressions::Initializer>();
  if (initializer) {
    auto completed = initializer->get_completed_values();
    auto lowered =
        completed ? lower_pack(*completed, *type, assembler)
                  : lower_pack(initializer->get_arguments(), *type, assembler);
    BAIL_IF(!lowered);
    Value value(expression, *type, lowered->id);
    BAIL_IF(!retain_value(value));
    return value;
  }
  reject(
      expression, "This Library Expression has no SPIR V lowering yet."_view);
  return {};
}

auto Module::Body::lower_return(
    const Library::Language::Model::Pack& pack,
    const Interface::Stage& stage,
    Assembler::SpirV& assembler) -> Bool {
  const auto& results = stage.function.get().get_signature().get_results();
  BAIL_IF(results.get_size() != stage.outputs.get_size());
  for (Count index = 0; index < results.get_size(); index++) {
    auto semantic = results.get_abstract(index);
    auto type = semantic
                    ? Types::select(*semantic)
                    : Core::Option<const Library::Language::Model::Type&>();
    BAIL_IF(!type);

    // A single aggregate result may accept several positional Pack values.
    // Preserve that complete fitted Pack so Structure lowering can construct
    // the target instead of selecting only its first scalar producer.
    if (results.get_size() == 1 && pack.fits(*type)) {
      auto value = lower_pack(pack, *type, assembler);
      BAIL_IF(!value);
      assembler.store(stage.outputs.get_view().get_data()[index].id, value->id);
      continue;
    }

    auto source_index = select_source(pack, results, index);
    auto child = source_index
                     ? select_scalar_pack(pack, *source_index)
                     : Core::Option<const Library::Language::Model::Pack&>();
    BAIL_IF(!child);
    auto value = lower_pack(*child, *type, assembler);
    BAIL_IF(!value);
    assembler.store(stage.outputs.get_view().get_data()[index].id, value->id);
  }
  return True;
}

auto Module::Body::emit(
    const Interface::Stage& stage,
    Assembler::SpirV& assembler) -> Bool {
  // Stage inputs enter as global variables and become ordinary Library values
  // after one load. Locals remain direct SSA values until mutation needs a
  // physical Function storage choice.
  values.clear();
  assembler.function(
      types.get_void_id(), stage.id, Assembler::SpirV::FunctionControl::None,
      types.get_function_id());
  assembler.label(ids.take());

  for (const Interface::Variable& input : stage.inputs.get_view()) {
    auto type_id = types.get_id(input.type.get());
    BAIL_IF(!type_id);
    U32 id = ids.take();
    assembler.load(*type_id, id, input.id);
    BAIL_IF(!retain_value(Value(input.semantic.get(), input.type.get(), id)));
  }

  for (const Interface::Variable& binding : interface.get_bindings()) {
    auto type_id = types.get_id(binding.type.get());
    auto pointer = interface.get_binding_pointer(binding, assembler);
    BAIL_IF(!type_id || !pointer);
    U32 id = ids.take();
    assembler.load(*type_id, id, *pointer);
    BAIL_IF(
        !retain_value(Value(binding.semantic.get(), binding.type.get(), id)));
  }

  Bool returned = False;
  auto body = stage.function.get().get_body();
  BAIL_IF(!body);
  for (const Library::Language::Statement& statement : body->get_statements()) {
    const Abstract& root = statement.get_root();
    auto local = root.select<Library::Language::Flow::Local>();
    if (local) {
      auto type = local->get_linked_type();
      auto initializer = local->get_initializer();
      auto value = type && initializer
                       ? lower_pack(*initializer, *type, assembler)
                       : Core::Option<Value>();
      if (!type || !value || !retain_value(Value(*local, *type, value->id))) {
        return reject(
            *local, "This Local could not produce its SPIR V value."_view);
      }
      continue;
    }
    auto return_statement = root.select<Library::Language::Flow::Return>();
    if (return_statement) {
      if (!lower_return(return_statement->get_pack(), stage, assembler)) {
        Tetrodotoxin::Source::Lexical::Errors::Report report(
            request.get_errors(), request.get_source_path(),
            request.get_source_text(), return_statement->get_anchor());
        report
            << "SPIR V lowering could not materialize this valid return Pack.\n"
               "Source produces: "_view;
        Library::Language::Diagnostics::write_pack(
            report, return_statement->get_pack());
        report << "\nStage accepts: "_view;
        Library::Language::Diagnostics::write_layout(
            report, stage.function.get().get_signature().get_results());
        report.get_hint()
            << "The Library return is valid; this is a missing target "
               "lowering."_view;
        return False;
      }
      assembler.return_void();
      returned = True;
      break;
    }
    auto expression = root.select<Library::Language::Expression>();
    if (expression) {
      BAIL_IF(!lower_expression(*expression, assembler));
      continue;
    }
    return reject(
        root, "This Library Statement has no SPIR V lowering yet."_view);
  }

  if (!returned) {
    BAIL_IF(!stage.outputs.is_empty());
    assembler.return_void();
  }
  assembler.function_end();
  return True;
}
