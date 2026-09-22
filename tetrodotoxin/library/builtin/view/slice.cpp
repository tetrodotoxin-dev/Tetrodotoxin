// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/builtin/view/slice.hpp"

#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/expression.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Library;

static auto create_parameter_entries(
    Tetrodotoxin::Source::Layouts::Addressable& self,
    Tetrodotoxin::Source::Layouts::Addressable& start,
    Tetrodotoxin::Source::Layouts::Addressable& count)
    -> Core::Static::Vector<Reference<const Abstract>, 3> {
  const Core::Static::Vector<Reference<const Abstract>, 3> entries = {{
    Reference<const Abstract>(self),
    Reference<const Abstract>(start),
    Reference<const Abstract>(count),
  }};
  return entries;
}

Builtin::View::Slice::Slice(
    Tetrodotoxin::Source::Layouts::Addressable& self,
    Tetrodotoxin::Source::Layouts::Addressable& start,
    Tetrodotoxin::Source::Layouts::Addressable& count,
    const Language::Model::Type& result)
    : parameter_entries(create_parameter_entries(self, start, count)),
      parameters(parameter_entries.get_view()),
      results(result, 1),
      result_type(result) {}

auto Builtin::View::Slice::create(
    Memory::Allocator::Arena& domain,
    const Language::Model::Type& receiver,
    const Language::Model::Type& count,
    const Language::Model::Type& result) -> Slice& {
  Tetrodotoxin::Source::Layouts::Addressable& self =
      Tetrodotoxin::Source::Layouts::Addressable::create_synthetic(
          domain, "self"_view, receiver);
  Tetrodotoxin::Source::Layouts::Addressable& start =
      Tetrodotoxin::Source::Layouts::Addressable::create_synthetic(
          domain, "start"_view, count);
  Tetrodotoxin::Source::Layouts::Addressable& size =
      Tetrodotoxin::Source::Layouts::Addressable::create_synthetic(
          domain, "count"_view, count);
  return domain.construct_from<Slice>(
      [&]() -> Slice { return Slice(self, start, size, result); });
}

static auto select_unsigned(const Tetrodotoxin::Source::Pack& values, Count index)
    -> Core::Option<U64> {
  auto producer = values.get_layout().get_abstract(index);
  BAIL_IF(!producer);
  auto constant = producer->select<Language::Constants::Unsigned>();
  if (constant) {
    return constant->get_value();
  }

  // The argument Pack retains its authored producer identity. Following that
  // producer through ordinary folding keeps const Locals usable without
  // copying their values into the Callable.
  auto expression =
      const_cast<Abstract&>(*producer).select<Language::Expression>();
  BAIL_IF(!expression);

  Core::Option<Language::Model::Pack&> folded;
  expression->fold().visit(
      [&](const Core::Option<Language::Model::Pack&>& selected) {
        folded = selected;
      },
      [](const Language::Expression::Error&) {});
  BAIL_IF(!folded);

  auto selected = folded->get_layout().get_abstract(0);
  BAIL_IF(!selected);
  constant = selected->select<Language::Constants::Unsigned>();
  return constant ? Core::Option<U64>(constant->get_value())
                  : Core::Option<U64>();
}

auto Builtin::View::Slice::fold_call(
    Memory::Allocator::Arena& domain,
    Core::Option<const Language::Model::Pack&> receiver,
    const Language::Model::Pack& arguments) const
    -> Core::Option<Language::Model::Pack&> {
  BAIL_IF(!receiver || arguments.get_layout().get_size() != 2);

  auto bytes = receiver->select_identity<Language::Constants::Bytes>();
  auto start = select_unsigned(arguments, 0);
  auto count = select_unsigned(arguments, 1);
  BAIL_IF(
      !bytes || !start || !count || *start > U64(Count(-1)) ||
      *count > U64(Count(-1)));

  Core::View::Bytes selected =
      bytes->get_value().slice(Count(*start), Count(*count));
  return Language::Constants::Bytes::create_synthetic(
      domain, result_type, selected);
}
