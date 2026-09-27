// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/object.hpp"
#include "perimortem/core/option.hpp"

namespace Tetrodotoxin::Graphics::Runtime {

// Children2D exposes only the ordered child Objects of one native Object. A
// child retains the configured Type index selected by the compiled Scene.
class Children2D {
 public:
  class Child {
   public:
    constexpr Child() = default;
    constexpr Child(Perimortem::Core::Object<> object, Count type_index)
        : object(object), type_index(type_index) {}

    constexpr auto get_object() const -> Perimortem::Core::Object<> {
      return object;
    }
    constexpr auto get_type_index() const -> Count { return type_index; }

   private:
    Perimortem::Core::Object<> object;
    Count type_index = Count(-1);
  };

  using ReadCount = Count (*)(const U8*, Perimortem::Core::Object<>);
  using Read = Count (*)(
      const U8*,
      Perimortem::Core::Object<>,
      Count,
      Perimortem::Core::Object<>*);

  constexpr Children2D(const U8* context, ReadCount read_count, Read read)
      : context(context), read_count(read_count), read(read) {}

  auto child_count(Perimortem::Core::Object<> object) const -> Count;
  auto child(Perimortem::Core::Object<> object, Count index) const
      -> Perimortem::Core::Option<Child>;

 private:
  const U8* context;
  ReadCount read_count;
  Read read;
};

}  // namespace Tetrodotoxin::Graphics::Runtime
