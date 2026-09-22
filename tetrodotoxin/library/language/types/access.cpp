// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/access.hpp"

#include "tetrodotoxin/library/builtin/view/is_empty.hpp"
#include "tetrodotoxin/library/builtin/view/size.hpp"
#include "tetrodotoxin/library/builtin/view/slice.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/source/unknown.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library::Language;

Types::Access::Access(
    Perimortem::Memory::Allocator::Arena& domain,
    View::Bytes name,
    const Model::Type& element,
    const Model::Type& size_type,
    const Model::Type& flag_type,
    const Model::Type& view_type)
    : name(name), element(element) {
  auto& get_size = Builtin::View::Size::create(domain, *this, size_type);
  auto& is_empty = Builtin::View::IsEmpty::create(domain, *this, flag_type);
  auto& slice =
      Builtin::View::Slice::create(domain, *this, size_type, view_type);
  publish_callable(domain, get_size, True);
  publish_callable(domain, is_empty, True);
  publish_callable(domain, slice, True);
}

auto Types::Access::create_default(
    Perimortem::Memory::Allocator::Arena& arena) const -> Option<Model::Pack&> {
  return Constants::Bytes::create_synthetic(arena, *this, {});
}
