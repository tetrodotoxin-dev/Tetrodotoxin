// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include <perimortem/memory/dynamic/bytes.hpp>

#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;
using namespace Perimortem::Memory;

static_assert(sizeof(Dynamic::Bytes) == sizeof(U8*) + sizeof(Count));
static_assert(alignof(Dynamic::Bytes) == alignof(Count));
static_assert(__is_standard_layout(Dynamic::Bytes));

auto main() -> int {
  Dynamic::Bytes value("alpha"_view);
  Dynamic::Bytes reserved(Count(32));
  reserved = value.get_view();
  if (!(reserved == "alpha"_view) || reserved.get_capacity() < 32) {
    return 1;
  }

  Dynamic::Bytes copied = reserved;
  Access::Bytes writable = copied;
  writable.get_data()[0] = 'A';
  if (!(reserved == "alpha"_view) || !(copied == "Alpha"_view)) {
    return 2;
  }

  View::Bytes borrowed = copied;
  if (!(borrowed == "Alpha"_view)) {
    return 3;
  }

  copied.append('!');
  copied.append('?', 2);
  copied.ensure_capacity(64);
  if (!(copied == "Alpha!??"_view) || copied.get_capacity() < 64) {
    return 4;
  }

  copied.set('x');
  copied.convert('x', 'y');
  if (!(copied == "yyyyyyyy"_view) || copied.at(Count(-1)) != 0 ||
      copied[0] != 'y') {
    return 5;
  }

  copied.proxy("prefix-data"_view);
  copied.shrink(7);
  if (copied.slice(0, 4) != "data"_view || copied.hash() == 0) {
    return 6;
  }

  copied.forgetful_resize(2);
  copied.resize(4);
  copied.clear();
  copied.reset();
  if (!copied.is_empty() || copied.get_size() != 0) {
    return 7;
  }

  Dynamic::Bytes joined = Dynamic::Bytes::concat("left"_view, "right"_view);
  Dynamic::Bytes cloned = Dynamic::Bytes::copy(joined.get_view());
  return joined == "leftright"_view && cloned == joined ? 0 : 8;
}
