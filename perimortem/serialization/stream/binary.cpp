// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/serialization/stream/binary.hpp"

#include "perimortem/core/writer/binary.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/managed/bytes.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;

template <Data::ByteOrder endian, typename storage_type, typename value_type>
static auto write(storage_type& storage, value_type value) -> void {
  const Count start = storage.get_size();
  storage.resize(start + sizeof(value));
  Core::Writer::Binary<endian> writer(
      storage.get_access().slice(start, sizeof(value)));
  writer << value;
}

template <Data::ByteOrder endian, typename storage_type, typename value_type>
static auto write_blob(storage_type& storage, View::Vector<value_type> value)
    -> void {
  if (value.is_empty()) {
    return;
  }

  const Count bytes = value.get_size() * sizeof(value_type);
  const Count start = storage.get_size();
  storage.resize(start + bytes);
  Core::Writer::Binary<endian> writer(storage.get_access().slice(start, bytes));
  writer << value;
}

template <Data::ByteOrder stream_endian, typename storage_type>
auto Stream::Binary<stream_endian, storage_type>::operator<<(U8 value)
    -> Binary& {
  write<stream_endian>(storage, value);
  return *this;
}

template <Data::ByteOrder stream_endian, typename storage_type>
auto Stream::Binary<stream_endian, storage_type>::operator<<(U16 value)
    -> Binary& {
  write<stream_endian>(storage, value);
  return *this;
}

template <Data::ByteOrder stream_endian, typename storage_type>
auto Stream::Binary<stream_endian, storage_type>::operator<<(U32 value)
    -> Binary& {
  write<stream_endian>(storage, value);
  return *this;
}

template <Data::ByteOrder stream_endian, typename storage_type>
auto Stream::Binary<stream_endian, storage_type>::operator<<(U64 value)
    -> Binary& {
  write<stream_endian>(storage, value);
  return *this;
}

template <Data::ByteOrder stream_endian, typename storage_type>
auto Stream::Binary<stream_endian, storage_type>::operator<<(S8 value)
    -> Binary& {
  write<stream_endian>(storage, value);
  return *this;
}

template <Data::ByteOrder stream_endian, typename storage_type>
auto Stream::Binary<stream_endian, storage_type>::operator<<(S16 value)
    -> Binary& {
  write<stream_endian>(storage, value);
  return *this;
}

template <Data::ByteOrder stream_endian, typename storage_type>
auto Stream::Binary<stream_endian, storage_type>::operator<<(S32 value)
    -> Binary& {
  write<stream_endian>(storage, value);
  return *this;
}

template <Data::ByteOrder stream_endian, typename storage_type>
auto Stream::Binary<stream_endian, storage_type>::operator<<(S64 value)
    -> Binary& {
  write<stream_endian>(storage, value);
  return *this;
}

template <Data::ByteOrder stream_endian, typename storage_type>
auto Stream::Binary<stream_endian, storage_type>::operator<<(R32 value)
    -> Binary& {
  write<stream_endian>(storage, value);
  return *this;
}

template <Data::ByteOrder stream_endian, typename storage_type>
auto Stream::Binary<stream_endian, storage_type>::operator<<(R64 value)
    -> Binary& {
  write<stream_endian>(storage, value);
  return *this;
}

template <Data::ByteOrder stream_endian, typename storage_type>
auto Stream::Binary<stream_endian, storage_type>::operator<<(View::Bytes blob)
    -> Binary& {
  storage.concat(blob);
  return *this;
}

template <Data::ByteOrder stream_endian, typename storage_type>
auto Stream::Binary<stream_endian, storage_type>::operator<<(
    View::Vector<U8> blob) -> Binary& {
  storage.concat(blob.get_bytes());
  return *this;
}

template <Data::ByteOrder stream_endian, typename storage_type>
auto Stream::Binary<stream_endian, storage_type>::operator<<(
    View::Vector<U16> blob) -> Binary& {
  write_blob<stream_endian>(storage, blob);
  return *this;
}

template <Data::ByteOrder stream_endian, typename storage_type>
auto Stream::Binary<stream_endian, storage_type>::operator<<(
    View::Vector<U32> blob) -> Binary& {
  write_blob<stream_endian>(storage, blob);
  return *this;
}

template <Data::ByteOrder stream_endian, typename storage_type>
auto Stream::Binary<stream_endian, storage_type>::operator<<(
    View::Vector<U64> blob) -> Binary& {
  write_blob<stream_endian>(storage, blob);
  return *this;
}

template <Data::ByteOrder stream_endian, typename storage_type>
auto Stream::Binary<stream_endian, storage_type>::operator<<(
    View::Vector<S8> blob) -> Binary& {
  write_blob<stream_endian>(storage, blob);
  return *this;
}

template <Data::ByteOrder stream_endian, typename storage_type>
auto Stream::Binary<stream_endian, storage_type>::operator<<(
    View::Vector<S16> blob) -> Binary& {
  write_blob<stream_endian>(storage, blob);
  return *this;
}

template <Data::ByteOrder stream_endian, typename storage_type>
auto Stream::Binary<stream_endian, storage_type>::operator<<(
    View::Vector<S32> blob) -> Binary& {
  write_blob<stream_endian>(storage, blob);
  return *this;
}

template <Data::ByteOrder stream_endian, typename storage_type>
auto Stream::Binary<stream_endian, storage_type>::operator<<(
    View::Vector<S64> blob) -> Binary& {
  write_blob<stream_endian>(storage, blob);
  return *this;
}

template class Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>;
template class Stream::Binary<Data::ByteOrder::Big, Dynamic::Bytes>;
template class Stream::Binary<Data::ByteOrder::Little, Managed::Bytes>;
template class Stream::Binary<Data::ByteOrder::Big, Managed::Bytes>;
