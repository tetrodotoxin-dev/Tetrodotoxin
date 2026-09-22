// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/hash.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/utility/pair.hpp"

namespace Perimortem::Memory::Dynamic {

// Unordered scalar hash map that owns keys and values by association.
//
// Entries live inline in one allocation beside their control hashes. Insert,
// remove, clear, and ensure_capacity may relocate entries, so references
// selected by find and pointers returned by get_entry remain valid only until
// the next mutating call.
//
// The table uses linear probing because compiler maps are normally small and
// benefit more from compact storage and a cheap header than from a second
// vectorized layout. Removal repairs the following probe cluster so lookup can
// still stop at the first empty bucket.
template <typename key_type, typename value_type>
class Map {
 public:
  using Entry = Utility::Pair<key_type, value_type>;

  constexpr Map() = default;
  constexpr Map(Count initial_capacity) { ensure_capacity(initial_capacity); }

  template <Count aggregate_size>
  Map(const Entry (&items)[aggregate_size]) {
    ensure_capacity(aggregate_size);
    for (Count i = 0; i < aggregate_size; i++) {
      insert(items[i]);
    }
  }

  Map(const Map& rhs) {
    ensure_capacity(rhs.get_size());
    for (Count i = 0; i < rhs.bucket_count; i++) {
      if (rhs.buckets[i] != 0) {
        insert(rhs.entries[i]);
      }
    }
  }

  Map(Map&& rhs)
      : buckets(rhs.buckets),
        entries(rhs.entries),
        bucket_count(rhs.bucket_count),
        size(rhs.size) {
    rhs.buckets = nullptr;
    rhs.entries = nullptr;
    rhs.bucket_count = 0;
    rhs.size = 0;
  }

  auto operator=(const Map& rhs) -> Map& {
    if (this == &rhs) {
      return *this;
    }

    clear();
    ensure_capacity(rhs.get_size());
    for (Count i = 0; i < rhs.bucket_count; i++) {
      if (rhs.buckets[i] != 0) {
        insert(rhs.entries[i]);
      }
    }

    return *this;
  }

  auto operator=(Map&& rhs) -> Map& {
    if (this != &rhs) {
      Core::Data::swap(buckets, rhs.buckets);
      Core::Data::swap(entries, rhs.entries);
      Core::Data::swap(bucket_count, rhs.bucket_count);
      Core::Data::swap(size, rhs.size);
    }

    return *this;
  }

  ~Map() {
    if (buckets == nullptr) {
      return;
    }

    destruct();
    Core::Bibliotheca::remit(Core::Data::cast<U8>(buckets));
  }

  auto ensure_capacity(Count items) -> void {
    if (buckets != nullptr && items * 10 <= Count(bucket_count) * 9) {
      return;
    }

    Count new_bucket_count = bucket_count == 0 ? 2 : bucket_count << 1;
    while (items * 10 > new_bucket_count * 9) {
      new_bucket_count <<= 1;
    }

    grow(new_bucket_count);
  }

  auto clear() -> void {
    destruct();
    size = 0;
  }

  auto insert(const Entry& item) -> Entry* {
    return insert(item.key, item.value);
  }

  auto insert(const key_type& key, const value_type& value) -> Entry* {
    U32 hash = get_hash(key);
    auto* found = find_hashed(key, hash);
    if (found != nullptr) {
      found->value = value;
      return found;
    }

    ensure_capacity(size + 1);
    Entry* empty = get_empty(hash);
    new (empty, Core::Placement::Construct) Entry(key, value);
    size++;
    return empty;
  }

  auto emplace(Entry&& item) -> Entry* {
    return emplace(
        static_cast<key_type&&>(item.key),
        static_cast<value_type&&>(item.value));
  }

  auto emplace(key_type&& key, value_type&& value) -> Entry* {
    U32 hash = get_hash(key);
    auto* found = find_hashed(key, hash);
    if (found != nullptr) {
      found->value = static_cast<value_type&&>(value);
      return found;
    }

    ensure_capacity(size + 1);
    Entry* empty = get_empty(hash);
    new (empty, Core::Placement::Construct)
        Entry(static_cast<key_type&&>(key), static_cast<value_type&&>(value));
    size++;
    return empty;
  }

  template <typename found_func, typename missing_func>
  constexpr auto
      visit(const key_type& key, found_func found, missing_func missing) const
      -> decltype(auto) {
    auto element = find(key);
    if (element) {
      return found((*element).value);
    } else {
      return missing();
    }
  }

  auto find(const key_type& key) -> Core::Option<Entry&> {
    auto* found = find_hashed(key, get_hash(key));
    if (found == nullptr) {
      return {};
    }

    return *found;
  }

  auto find(const key_type& key) const -> Core::Option<const Entry&> {
    const auto* found = find_hashed(key, get_hash(key));
    if (found == nullptr) {
      return {};
    }

    return *found;
  }

  auto contains(const key_type& key) const -> Bool { return bool(find(key)); }

  auto remove(const key_type& key) -> Bool {
    Count bucket = find_bucket(key, get_hash(key));
    if (bucket == Count(-1)) {
      return False;
    }

    Count bucket_mask = bucket_count - 1;
    Count next = (bucket + 1) & bucket_mask;
    Count hole = bucket;
    buckets[bucket] = 0;

    // The removed entry remains alive in the hole until the probe cluster has
    // been repaired. Moving a displaced entry backward carries that removed
    // object forward, leaving exactly one object to destroy at the final hole.
    while (buckets[next] != 0) {
      U32 hash = buckets[next];
      Count home = extract_bucket_index(hash);
      Count entry_distance = (next - home) & bucket_mask;
      Count hole_distance = (hole - home) & bucket_mask;
      if (hole_distance < entry_distance) {
        buckets[hole] = hash;
        buckets[next] = 0;
        Core::Data::swap(entries[hole], entries[next]);
        hole = next;
      }

      next = (next + 1) & bucket_mask;
    }

    destruct_entry(entries + hole);
    size--;
    return True;
  }

  auto get_entry(Count index) -> Entry* {
    return const_cast<Entry*>(static_cast<const Map*>(this)->get_entry(index));
  }

  auto get_entry(Count index) const -> const Entry* {
    if (index >= size) {
      return nullptr;
    }

    for (Count i = 0; i < bucket_count; i++) {
      if (buckets[i] == 0) {
        continue;
      }

      if (index == 0) {
        return entries + i;
      }

      index--;
    }

    return nullptr;
  }

  auto at(const key_type& key) -> value_type& {
    U32 hash = get_hash(key);
    auto* found = find_hashed(key, hash);
    if (found != nullptr) {
      return found->value;
    }

    ensure_capacity(size + 1);
    Entry* entry = get_empty(hash);
    new (entry, Core::Placement::Construct) Entry(key, value_type());
    size++;
    return entry->value;
  }

  auto operator[](const key_type& key) -> value_type& { return at(key); }

  auto find_or_default(const key_type& key, const value_type& value) const
      -> const value_type& {
    auto entry = find(key);
    return entry ? (*entry).value : value;
  }

  constexpr auto get_size() const -> Count { return size; }
  constexpr auto get_capacity() const -> Count { return bucket_count; }
  constexpr auto is_empty() const -> Bool { return size == 0; }

 private:
  auto get_empty(U32 hash) -> Entry* {
    Count bucket = extract_bucket_index(hash);
    while (buckets[bucket] != 0) {
      bucket = (bucket + 1) & (bucket_count - 1);
    }

    buckets[bucket] = extract_bucket_key(hash);
    return entries + bucket;
  }

  auto find_bucket(const key_type& key, U32 hash) const -> Count {
    // Both views belong to one allocation. Incomplete storage has no entries.
    if (size == 0 || buckets == nullptr || entries == nullptr) {
      return Count(-1);
    }

    U32 bucket_key = extract_bucket_key(hash);
    Count bucket = extract_bucket_index(hash);
    while (True) {
      U32 stored_key = buckets[bucket];
      if (stored_key == bucket_key && entries[bucket].key == key) {
        return bucket;
      }

      if (stored_key == 0) {
        return Count(-1);
      }

      bucket = (bucket + 1) & (bucket_count - 1);
    }
  }

  auto find_hashed(const key_type& key, U32 hash) -> Entry* {
    if (entries == nullptr) {
      return nullptr;
    }

    Count bucket = find_bucket(key, hash);
    if (bucket == Count(-1)) {
      return nullptr;
    }

    return entries + bucket;
  }

  auto find_hashed(const key_type& key, U32 hash) const -> const Entry* {
    if (entries == nullptr) {
      return nullptr;
    }

    Count bucket = find_bucket(key, hash);
    if (bucket == Count(-1)) {
      return nullptr;
    }

    return entries + bucket;
  }

  auto emplace_hashed(Entry* entry, U32 hash) -> void {
    Entry* empty = get_empty(hash);
    memcpy(Core::Data::cast<void>(empty), entry, sizeof(Entry));
  }

  static auto destruct_entry(Entry* entry) -> void {
    entry->key.~key_type();
    entry->value.~value_type();
  }

  auto destruct() -> void {
    if (buckets == nullptr) {
      return;
    }

    for (Count i = 0; i < bucket_count; i++) {
      if (buckets[i] == 0) {
        continue;
      }

      destruct_entry(entries + i);
      buckets[i] = 0;
    }
  }

  auto grow(Count new_bucket_count) -> void {
    U32* old_buckets = buckets;
    Entry* old_entries = entries;
    Count old_bucket_count = bucket_count;
    Count old_size = size;
    create_buffer(new_bucket_count);
    if (old_buckets != nullptr && old_entries != nullptr) {
      for (Count i = 0; i < old_bucket_count; i++) {
        if (old_buckets[i] != 0) {
          emplace_hashed(old_entries + i, old_buckets[i]);
        }
      }
    }

    if (old_buckets != nullptr) {
      Core::Bibliotheca::remit(Core::Data::cast<U8>(old_buckets));
    }

    size = old_size;
  }

  auto create_buffer(Count new_bucket_count) -> void {
    Core::Bibliotheca::Allocation allocation =
        Core::Bibliotheca::check_out(required_buffer_size(new_bucket_count));
    buckets = Core::Data::cast<U32>(allocation.ptr);
    entries = Core::Data::cast<Entry>(
        allocation.ptr + entry_offset(new_bucket_count));
    bucket_count = new_bucket_count;
    size = 0;
    for (Count i = 0; i < new_bucket_count; i++) {
      buckets[i] = 0;
    }
  }

  static constexpr auto required_buffer_size(Count bucket_count) -> Count {
    return entry_offset(bucket_count) + sizeof(Entry) * bucket_count;
  }

  static constexpr auto entry_offset(Count bucket_count) -> Count {
    return Core::Data::align<alignof(Entry)>(sizeof(U32) * bucket_count);
  }

  constexpr auto extract_bucket_index(U32 hash) const -> Count {
    return hash & (bucket_count - 1);
  }

  static constexpr auto extract_bucket_key(U32 hash) -> U32 {
    return hash | U32(0x80000000);
  }

  static auto get_hash(const key_type& key) -> U32 {
    return U32(Core::Hash(key).get_value());
  }

  U32* buckets = nullptr;
  Entry* entries = nullptr;
  U32 bucket_count = 0;
  U32 size = 0;
};

}  // namespace Perimortem::Memory::Dynamic
