// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/core/reader/serial.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;

constexpr auto native_endian = Data::ByteOrder::Native;
constexpr auto stream_endian = Data::ByteOrder::Little;

constexpr auto negate_flag = 0x10;
constexpr auto blob_flag = 0x20;

auto Reader::Serial::read() -> Value {
  if (!has_content()) {
    Diagnostics::Log::Message<128> error_message(
        Diagnostics::Log::Level::Error);
    error_message
        << "Serial read overran data buffer while reading type at byte location "_view
        << cursor << ". source_size="_view << source.get_size();

    cursor = Count(-1);
    return Value();
  }

  auto data = source.get_data();
  auto byte_flag = data[cursor++];
  U8 encoded_size = byte_flag & 0xF;
  if (cursor + encoded_size > source.get_size()) {
    Diagnostics::Log::Message<128> error_message(
        Diagnostics::Log::Level::Error);
    error_message
        << "Serial read overran data buffer while reading value at byte location "_view
        << cursor << ". source_size="_view << source.get_size()
        << " encoded_size="_view << encoded_size;

    cursor = Count(-1);
    return Value();
  }

  S64 value;
  switch (encoded_size) {
  case 1:
    value = source[cursor];
    break;
  case 2: {
    U16 actual_bytes;
    memcpy(&actual_bytes, data + cursor, sizeof(U16));
    value = Data::ensure_endian<stream_endian, native_endian>(actual_bytes);
    break;
  }
  case 4: {
    U32 actual_bytes;
    memcpy(&actual_bytes, data + cursor, sizeof(U32));
    value = Data::ensure_endian<stream_endian, native_endian>(actual_bytes);
    break;
  }
  case 8:
    memcpy(&value, data + cursor, sizeof(U64));
    value = Data::ensure_endian<stream_endian, native_endian>(value);
    break;
  default: {
    Diagnostics::Log::Message<128> error_message(
        Diagnostics::Log::Level::Error);
    error_message << "Serial read found invalid encoding size "_view
                  << (encoded_size) << " at byte location "_view << cursor
                  << "."_view;

    cursor = Count(-1);
    return Value();
  }
  }

  // Bump the cursor by the encoded size.
  cursor += encoded_size;

  // Take the twos complement of the value if the negate flag is set.
  if (byte_flag & negate_flag) {
    value = -value;
  }

  // If it's not a blob then we can return the value directly.
  if (!(byte_flag & blob_flag)) {
    return Value(value);
  }

  if (cursor + value > source.get_size()) {
    Diagnostics::Log::Message<128> error_message(
        Diagnostics::Log::Level::Error);
    error_message << "Serial read of blob sized "_view << value
                  << " bytes overran source buffer at location "_view << cursor
                  << ". source_size="_view << source.get_size()
                  << " blob_size="_view << value;

    cursor = Count(-1);
    return Value();
  }

  // Bump the cursor to include the blob as well.
  auto blob_start = data + cursor;
  cursor += value;
  return View::Bytes(blob_start, value);
}

auto Reader::Serial::read_value() -> S64 {
  // If attribution isn't set then make it explicit any failures were from the
  // read_value call rather than just read.
  auto attribution = Diagnostics::Log::set_attribution();
  return read().get_value();
}

auto Reader::Serial::read_blob() -> View::Bytes {
  // If attribution isn't set then make it explicit any failures were from the
  // read_blob call rather than just read.
  auto attribution = Diagnostics::Log::set_attribution();
  return read().get_view();
}
