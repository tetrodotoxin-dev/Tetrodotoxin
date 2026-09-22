// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/linker/elf/object.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/writer/binary.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Linker;

using LittleWriter = Core::Writer::Binary<Core::Data::ByteOrder::Little>;

static constexpr Count elf_header_size = 64;
static constexpr Count section_header_size = 64;
static constexpr Count symbol_record_size = 24;
static constexpr Count section_count = 5;
static constexpr U16 read_only_section = 1;
static constexpr U16 string_table_section = 3;
static constexpr U16 section_names_section = 4;

static constexpr auto align(Count value, Count alignment) -> Count {
  return (value + alignment - 1) & ~(alignment - 1);
}

static auto has_nul(Core::View::Bytes value) -> Bool {
  for (Count index = 0; index < value.get_size(); index++) {
    if (value[index] == 0) {
      return True;
    }
  }
  return False;
}

static auto write_section(
    LittleWriter& writer,
    U32 name,
    U32 type,
    U64 flags,
    U64 offset,
    U64 size,
    U32 link,
    U32 info,
    U64 alignment,
    U64 entry_size) -> void {
  writer << name << type << flags << U64(0) << offset << size << link << info
         << alignment << entry_size;
}

auto Elf::Object::add_read_only(
    Core::View::Bytes symbol,
    Core::View::Bytes end_symbol,
    Core::View::Bytes contents) -> Bool {
  BAIL_IF(
      symbol.is_empty() || end_symbol.is_empty() || has_nul(symbol) ||
      has_nul(end_symbol) ||
      contents.get_size() > U32(-1) - read_only.get_size() - 3);
  auto retained_symbols = symbols.get_view();
  for (Count index = 0; index < retained_symbols.get_size(); index++) {
    const Symbol& existing = retained_symbols.get_data()[index];
    BAIL_IF(existing.get_name() == symbol || existing.get_name() == end_symbol);
  }

  while (read_only.get_size() % 4 != 0) {
    read_only.append(0);
  }
  Count offset = read_only.get_size();
  read_only.concat(contents);
  symbols.insert(Symbol(symbol, offset, contents.get_size()));
  symbols.insert(Symbol(end_symbol, read_only.get_size(), 0));
  return True;
}

auto Elf::Object::build() const -> Core::Option<Memory::Dynamic::Bytes> {
  Memory::Dynamic::Bytes string_table;
  string_table.append(0);
  Memory::Dynamic::Vector<Count> name_offsets;
  auto retained_symbols = symbols.get_view();
  for (Count index = 0; index < retained_symbols.get_size(); index++) {
    const Symbol& symbol = retained_symbols.get_data()[index];
    BAIL_IF(
        string_table.get_size() > U32(-1) ||
        symbol.get_name().get_size() >= U32(-1) - string_table.get_size() ||
        symbol.get_offset() > read_only.get_size() ||
        symbol.get_size() > read_only.get_size() - symbol.get_offset());
    name_offsets.insert(string_table.get_size());
    string_table.concat(symbol.get_name());
    string_table.append(0);
  }
  BAIL_IF(string_table.get_size() > U32(-1));
  BAIL_IF(symbols.get_size() > U32(-1) / symbol_record_size - 1);

  constexpr Core::Static::Bytes<35> section_names = {{
    0,   '.', 'r', 'o', 'd', 'a', 't', 'a', 0,   '.', 's', 'y',
    'm', 't', 'a', 'b', 0,   '.', 's', 't', 'r', 't', 'a', 'b',
    0,   '.', 's', 'h', 's', 't', 'r', 't', 'a', 'b', 0,
  }};

  Count read_only_offset = read_only.is_empty() ? 0 : align(elf_header_size, 4);
  Count symbol_table_offset = align(
      read_only.is_empty() ? elf_header_size
                           : read_only_offset + read_only.get_size(),
      8);
  Count symbol_table_size = symbol_record_size * (symbols.get_size() + 1);
  Count string_table_offset = symbol_table_offset + symbol_table_size;
  Count section_names_offset = string_table_offset + string_table.get_size();
  Count section_headers_offset =
      align(section_names_offset + section_names.get_size(), 8);
  BAIL_IF(
      section_headers_offset > Count(-1) - section_header_size * section_count);
  Count file_size =
      section_headers_offset + section_header_size * section_count;

  Memory::Dynamic::Bytes output;
  output.forgetful_resize(file_size);
  output.set(0);
  LittleWriter writer(output.get_access());

  constexpr Core::Static::Bytes<16> identity = {{
    0x7F,
    'E',
    'L',
    'F',
    2,
    1,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
  }};
  writer << identity.get_view() << U16(1) << U16(62) << U32(1) << U64(0)
         << U64(0) << U64(section_headers_offset) << U32(0)
         << U16(elf_header_size) << U16(0) << U16(0) << U16(section_header_size)
         << U16(section_count) << U16(section_names_section);

  if (!read_only.is_empty()) {
    Core::Data::copy(
        output.get_access().get_data() + read_only_offset,
        read_only.get_view().get_data(), read_only.get_size());
  }

  writer.set_pointer(symbol_table_offset + symbol_record_size);
  auto retained_name_offsets = name_offsets.get_view();
  for (Count index = 0; index < retained_symbols.get_size(); index++) {
    const Symbol& symbol = retained_symbols.get_data()[index];
    writer << U32(retained_name_offsets.get_data()[index]) << U8(0x11) << U8(0)
           << U16(read_only_section) << U64(symbol.get_offset())
           << U64(symbol.get_size());
  }

  Core::Data::copy(
      output.get_access().get_data() + string_table_offset,
      string_table.get_view().get_data(), string_table.get_size());
  Core::Data::copy(
      output.get_access().get_data() + section_names_offset,
      section_names.get_data(), section_names.get_size());

  writer.set_pointer(section_headers_offset + section_header_size);
  write_section(
      writer, 1, 1, 2, read_only_offset, read_only.get_size(), 0, 0, 4, 0);
  write_section(
      writer, 9, 2, 0, symbol_table_offset, symbol_table_size,
      string_table_section, 1, 8, symbol_record_size);
  write_section(
      writer, 17, 3, 0, string_table_offset, string_table.get_size(), 0, 0, 1,
      0);
  write_section(
      writer, 25, 3, 0, section_names_offset, section_names.get_size(), 0, 0, 1,
      0);
  BAIL_IF(!writer.is_valid());
  return output;
}
