// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/shader/archive/writer.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "perimortem/core/writer/binary.hpp"

#include "perimortem/serialization/stream/binary.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin;

enum class ShaderWriterAttributeValue : U8 {
  Empty,
  Bytes,
  Unsigned,
  Signed,
  Real,
  Flag,
};

Shader::Archive::Writer::Writer() {
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes> appender(bytes);
  appender << "TTXS"_view;
  appender << U16(2);
  appender << U16(0);
}

auto Shader::Archive::Writer::encode(
    const Shader::Language::Monograph& monograph) -> Option<Dynamic::Bytes> {
  BAIL_IF(!monograph.is_finalized());

  Writer writer;
  auto record = writer.begin(Tag::Monograph);
  BAIL_IF(!writer.write(monograph.get_documentation()));
  for (const Reference<Shader::Language::Program>& retained :
       monograph.get_programs()) {
    BAIL_IF(!writer.write(retained.get()));
  }
  for (const Reference<Shader::Language::Bridge>& retained :
       monograph.get_bridges()) {
    BAIL_IF(!writer.write(retained.get()));
  }
  BAIL_IF(!writer.finish(record));
  return Data::take(writer.bytes);
}

auto Shader::Archive::Writer::begin(Tag tag) -> Record {
  Count offset = bytes.get_size();
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes> appender(bytes);
  appender << U16(tag);
  appender << U16(0);
  appender << U32(0);
  return Record(offset);
}

auto Shader::Archive::Writer::finish(Record record) -> Bool {
  Count offset = record.get_offset();
  BAIL_IF(offset > bytes.get_size() || bytes.get_size() - offset < 8);
  Count size = bytes.get_size() - offset - 8;
  BAIL_IF(size > U32(-1));
  Perimortem::Core::Writer::Binary<Data::ByteOrder::Little> patcher(
      bytes.get_access().slice(offset + 4, 4));
  patcher << U32(size);
  return patcher.is_valid();
}

auto Shader::Archive::Writer::write(U8 value) -> void {
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>(bytes) << value;
}

auto Shader::Archive::Writer::write(U32 value) -> void {
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>(bytes) << value;
}

auto Shader::Archive::Writer::write(U64 value) -> void {
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>(bytes) << value;
}

auto Shader::Archive::Writer::write(S64 value) -> void {
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>(bytes) << value;
}

auto Shader::Archive::Writer::write(R64 value) -> void {
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>(bytes) << value;
}

auto Shader::Archive::Writer::write(View::Bytes value) -> Bool {
  BAIL_IF(value.get_size() > U32(-1));
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes> appender(bytes);
  appender << U32(value.get_size());
  appender << value;
  return True;
}

auto Shader::Archive::Writer::write(const Tetrodotoxin::Source::Documentation& value) -> Bool {
  BAIL_IF(value.line_count() > U32(-1));
  write(U32(value.line_count()));
  for (Count index = 0; index < value.line_count(); index++) {
    BAIL_IF(!write(value.get_line(index)));
  }
  return True;
}

auto Shader::Archive::Writer::write(
    View::Vector<Tetrodotoxin::Language::Attribute> attributes) -> Bool {
  BAIL_IF(attributes.get_size() > U32(-1));
  write(U32(attributes.get_size()));
  for (const Tetrodotoxin::Language::Attribute& attribute : attributes) {
    BAIL_IF(!write(attribute.get_key()));
    Bool written = attribute.get_value().visit(
        [&]() -> Bool {
          write(U8(ShaderWriterAttributeValue::Empty));
          return True;
        },
        [&](View::Bytes selected) -> Bool {
          write(U8(ShaderWriterAttributeValue::Bytes));
          return write(selected);
        },
        [&](U64 selected) -> Bool {
          write(U8(ShaderWriterAttributeValue::Unsigned));
          write(selected);
          return True;
        },
        [&](S64 selected) -> Bool {
          write(U8(ShaderWriterAttributeValue::Signed));
          write(selected);
          return True;
        },
        [&](R64 selected) -> Bool {
          write(U8(ShaderWriterAttributeValue::Real));
          write(selected);
          return True;
        },
        [&](Bool selected) -> Bool {
          write(U8(ShaderWriterAttributeValue::Flag));
          write(U8(selected ? 1 : 0));
          return True;
        });
    BAIL_IF(!written);
  }
  return True;
}

auto Shader::Archive::Writer::write(
    const Tetrodotoxin::Language::Definition& definition) -> Bool {
  BAIL_IF(
      !write(definition.get_documentation()) ||
      !write(definition.get_attributes()) || !write(definition.get_name()));
  write(U8(definition.get_visibility()));
  return True;
}

auto Shader::Archive::Writer::write(const Shader::Language::Program& program)
    -> Bool {
  // Program declarations require the deferred Library projection serializer.
  return False;
}

auto Shader::Archive::Writer::write(const Shader::Language::Bridge& bridge)
    -> Bool {
  // Bridge references require the deferred Package dependency serializer.
  return False;
}
