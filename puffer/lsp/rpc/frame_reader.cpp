// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/lsp/rpc/frame_reader.hpp"

#include "perimortem/core/algorithm/search.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/reader/textual.hpp"

using namespace Perimortem::Core;
using namespace Puffer;

auto Lsp::Rpc::FrameReader::receive(View::Bytes bytes) -> void {
  data_stream.concat(bytes);
}

auto Lsp::Rpc::FrameReader::read_header() -> Bool {
  if (header_found) {
    return True;
  }

  Count break_location =
      Algorithm::search(data_stream.get_view(), "\r\n\r\n"_view);
  if (break_location == Count(-1)) {
    return False;
  }

  constexpr auto header_info = "Content-Length:"_view;
  Count content_size_location =
      Algorithm::search(data_stream.get_view(), header_info);
  if (content_size_location != Count(-1)) {
    const auto value_start = content_size_location + header_info.get_size();
    Reader::Textual count_reader(data_stream.get_view().slice(value_start));
    data_bytes_to_read = count_reader.read_unsigned();
    header_found = True;
  }

  data_stream.shrink(break_location + "\r\n\r\n"_view.get_size());
  return header_found;
}

auto Lsp::Rpc::FrameReader::next_message() -> View::Bytes {
  Bool header_read = read_header();
  if (!header_read) {
    return View::Bytes();
  }

  if (data_stream.get_size() < data_bytes_to_read) {
    return View::Bytes();
  }

  return data_stream.slice(0, data_bytes_to_read);
}

auto Lsp::Rpc::FrameReader::consume_message() -> void {
  if (!header_found || data_stream.get_size() < data_bytes_to_read) {
    return;
  }

  data_stream.shrink(data_bytes_to_read);
  header_found = False;
  data_bytes_to_read = 0;
}
