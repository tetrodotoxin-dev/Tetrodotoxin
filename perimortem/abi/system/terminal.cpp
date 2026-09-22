// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/abi/system/terminal.hpp"

#include "perimortem/system/terminal.hpp"

#include "perimortem/abi/core/object.hpp"

using namespace Perimortem;

extern "C" auto perimortem_system_terminal_read_line()
    -> Abi::Core::Option<Abi::Memory::Dynamic::Bytes> {
  System::Terminal terminal;
  auto line = terminal.read_line();
  if (!line) {
    return Abi::Core::Option<Abi::Memory::Dynamic::Bytes>::create();
  }

  Abi::Memory::Dynamic::Bytes result = Abi::Memory::Dynamic::Bytes::create(
      line->get_view().get_data(), line->get_size());
  perimortem_core_object_retain(const_cast<U8*>(result.get_data()));
  return Abi::Core::Option<Abi::Memory::Dynamic::Bytes>::create(result);
}

extern "C" auto perimortem_system_terminal_write_line(
    Abi::Memory::Dynamic::Bytes data) -> bool {
  System::Terminal terminal;
  return bool(
      terminal.write_line(Core::View::Bytes(data.get_data(), data.get_size())));
}
