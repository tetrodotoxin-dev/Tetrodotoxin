// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/scene/archive/writer.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "perimortem/serialization/stream/binary.hpp"


using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin;

Scene::Archive::Writer::Writer() {
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes> appender(bytes);
  appender << "TTSC"_view;
  appender << U16(3);
  appender << U16(0);
}

auto Scene::Archive::Writer::encode(const Scene::Language::Monograph& monograph)
    -> Option<Dynamic::Bytes> {
  // Scene packaging requires the deferred Library projection serializer.
  return {};
}

auto Scene::Archive::Writer::write(U8 value) -> void {
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>(bytes) << value;
}

auto Scene::Archive::Writer::write(U16 value) -> void {
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>(bytes) << value;
}

auto Scene::Archive::Writer::write(U32 value) -> void {
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>(bytes) << value;
}

auto Scene::Archive::Writer::write(View::Bytes value) -> Bool {
  BAIL_IF(value.get_size() > U32(-1));
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes> appender(bytes);
  appender << U32(value.get_size());
  appender << value;
  return True;
}

auto Scene::Archive::Writer::write(const Tetrodotoxin::Source::Documentation& documentation) -> Bool {
  BAIL_IF(documentation.line_count() > U32(-1));
  write(U32(documentation.line_count()));
  for (Count index = 0; index < documentation.line_count(); index++) {
    BAIL_IF(!write(documentation.get_line(index)));
  }
  return True;
}
