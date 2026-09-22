// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/hash.hpp"

static Count default_construct_count = 0;
static Count default_destruct_count = 0;

class Hashable {
 public:
  Hashable()
      : id(0),
        construct_count(default_construct_count),
        destruct_count(default_destruct_count) {}
  Hashable(Count id, Count& construct_count, Count& destruct_count)
      : id(id),
        construct_count(construct_count),
        destruct_count(destruct_count) {
    this->construct_count++;
  }

  Hashable(const Hashable& rhs)
      : id(rhs.id),
        construct_count(rhs.construct_count),
        destruct_count(rhs.destruct_count) {
    this->construct_count++;
  }

  Hashable(const Hashable&& rhs)
      : id(rhs.id),
        construct_count(rhs.construct_count),
        destruct_count(rhs.destruct_count) {
    this->construct_count++;
  }

  auto operator=(const Hashable& rhs) -> Hashable& {
    this->construct_count++;
    id = rhs.id;
    return *this;
  }

  ~Hashable() { destruct_count++; }
  auto hash() const -> U64 { return Perimortem::Core::Hash(id).get_value(); }

  auto operator==(const Hashable& rhs) const -> Bool { return rhs.id == id; }

 private:
  Count id;
  Count& construct_count;
  Count& destruct_count;
};
