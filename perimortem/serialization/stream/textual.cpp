// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/serialization/stream/textual.hpp"

#include "perimortem/core/writer/textual.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/managed/bytes.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;

// All textual operators are well under 64 bytes except Blobs which use
// `concat`. The write_window is just meant to bump us up to the next page size
// in the event we are close to a boundary. This can result in streams creating
// a lot of buffer waste when close to powers of 2, but generally Streams should
// only be used when the inputs are very dynamic so it shouldn't be banking on
// that anyway.
constexpr Count write_window = 64;

template <typename storage_type, typename value_type>
static auto write(storage_type& storage, value_type value) -> void {
  Count start = storage.get_size();
  storage.resize(start + write_window);

  // Use the Core::Writer to perform the actual serialization. Once that's done
  // we can shrink down to the actual serialized size.
  Core::Writer::Textual writer(storage.get_access().slice(start, write_window));
  writer << value;
  storage.resize(start + writer.get_location());
}

template <typename storage_type>
auto Stream::Textual<storage_type>::operator<<(Bool flag) -> Textual& {
  write(storage, flag);
  return *this;
}

template <typename storage_type>
auto Stream::Textual<storage_type>::operator<<(U8 value) -> Textual& {
  write(storage, value);
  return *this;
}

template <typename storage_type>
auto Stream::Textual<storage_type>::operator<<(U16 value) -> Textual& {
  write(storage, value);
  return *this;
}

template <typename storage_type>
auto Stream::Textual<storage_type>::operator<<(U32 value) -> Textual& {
  write(storage, value);
  return *this;
}

template <typename storage_type>
auto Stream::Textual<storage_type>::operator<<(U64 value) -> Textual& {
  write(storage, value);
  return *this;
}

template <typename storage_type>
auto Stream::Textual<storage_type>::operator<<(S8 value) -> Textual& {
  write(storage, value);
  return *this;
}

template <typename storage_type>
auto Stream::Textual<storage_type>::operator<<(S16 value) -> Textual& {
  write(storage, value);
  return *this;
}

template <typename storage_type>
auto Stream::Textual<storage_type>::operator<<(S32 value) -> Textual& {
  write(storage, value);
  return *this;
}

template <typename storage_type>
auto Stream::Textual<storage_type>::operator<<(S64 value) -> Textual& {
  write(storage, value);
  return *this;
}

template <typename storage_type>
auto Stream::Textual<storage_type>::operator<<(R32 value) -> Textual& {
  write(storage, value);
  return *this;
}

template <typename storage_type>
auto Stream::Textual<storage_type>::operator<<(R64 value) -> Textual& {
  write(storage, value);
  return *this;
}

template <typename storage_type>
auto Stream::Textual<storage_type>::operator<<(View::Bytes raw) -> Textual& {
  storage.concat(raw);
  return *this;
}

template class Stream::Textual<Dynamic::Bytes>;
template class Stream::Textual<Managed::Bytes>;
