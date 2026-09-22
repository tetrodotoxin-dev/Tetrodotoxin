// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/archive/writer.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "perimortem/serialization/stream/binary.hpp"

#include "tetrodotoxin/language/dialect.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Serialization;
using namespace Tetrodotoxin;

static auto fits_u32(Count value) -> Bool {
  return value <= Count(U32(-1));
}

static auto write_bytes(
    Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>& writer,
    View::Bytes value) -> Bool {
  BAIL_IF(!fits_u32(value.get_size()));
  writer << U32(value.get_size()) << value;
  return True;
}

static auto write_members(
    Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>& writer,
    View::Vector<Package::Archive::Member> members) -> Bool {
  BAIL_IF(!fits_u32(members.get_size()));
  writer << U32(members.get_size());
  for (const Package::Archive::Member& member : members) {
    BAIL_IF(
        !write_bytes(writer, member.get_semantic_name()) ||
        !write_bytes(writer, member.get_dialect_name()) ||
        !write_bytes(writer, member.get_payload()));
  }
  return True;
}

static auto write_resources(
    Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>& writer,
    View::Vector<Package::Archive::Resource> resources) -> Bool {
  BAIL_IF(!fits_u32(resources.get_size()));
  writer << U32(resources.get_size());
  for (const Package::Archive::Resource& resource : resources) {
    BAIL_IF(
        !write_bytes(writer, resource.get_route()) ||
        !write_bytes(writer, resource.get_value()));
  }
  return True;
}

static auto write_imports(
    Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes>& writer,
    View::Vector<Package::Archive::GraphImport> imports) -> Bool {
  BAIL_IF(!fits_u32(imports.get_size()));
  writer << U32(imports.get_size());
  for (const Package::Archive::GraphImport& import : imports) {
    writer << U8(import.get_kind()) << U8(import.get_visibility());
    BAIL_IF(
        !write_bytes(writer, import.get_importer()) ||
        !write_bytes(writer, import.get_local_name()) ||
        !write_bytes(writer, import.get_target()));
    writer << import.get_version().get_major()
           << import.get_version().get_minor();
    BAIL_IF(!write_bytes(writer, import.get_route()));
  }
  return True;
}

auto Package::Archive::Writer::write(const Archive& archive)
    -> Option<Dynamic::Bytes> {
  Dynamic::Bytes body;
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes> body_writer(body);
  BAIL_IF(!write_bytes(body_writer, archive.get_identity()));
  body_writer << archive.get_version().get_major()
              << archive.get_version().get_minor();
  BAIL_IF(
      !write_members(body_writer, archive.get_members()) ||
      !write_resources(body_writer, archive.get_resources()) ||
      !write_imports(body_writer, archive.get_imports()) ||
      !fits_u32(body.get_size()));

  Dynamic::Bytes output;
  Stream::Binary<Data::ByteOrder::Little, Dynamic::Bytes> writer(output);
  writer << "TTXA"_view << U16(Archive::format) << U16(0)
         << U32(body.get_size()) << body.get_view();
  return static_cast<Dynamic::Bytes&&>(output);
}

auto Package::Archive::Writer::write(
    const Package::Language::Monograph& package,
    View::Bytes identity,
    Perimortem::System::Version version,
    View::Vector<GraphMember> graph,
    View::Vector<GraphImport> imports) -> Option<Dynamic::Bytes> {
  BAIL_IF(identity.is_empty());

  Allocator::Arena arena;
  Dynamic::Vector<Dynamic::Bytes> payloads(graph.get_size() + 1);
  Managed::Vector<Member> members(arena);
  auto package_library = package.get_library()
                             .get_language()
                             .select<Tetrodotoxin::Language::Dialect>();
  BAIL_IF(!package_library);
  auto package_payload = package_library->encode(package.get_library());
  BAIL_IF(!package_payload);
  payloads.emplace(static_cast<Dynamic::Bytes&&>(*package_payload));
  members.insert(Member(
      "PackageSurface"_view, package_library->get_name(),
      payloads[payloads.get_size() - 1].get_view()));

  for (const GraphMember& selected : graph) {
    const Tetrodotoxin::Language::Monograph& member = selected.get_monograph();
    auto dialect =
        member.get_language().select<Tetrodotoxin::Language::Dialect>();
    BAIL_IF(!dialect);
    auto payload = dialect->encode(member);
    if (!payload) {
      Diagnostics::Log::Message<256> message(
          Diagnostics::Log::Level::Error, Diagnostics::Source());
      message << "Package Archive could not encode `"_view
              << selected.get_name() << "` with the "_view
              << dialect->get_name() << " Dialect."_view;
      return {};
    }
    payloads.emplace(static_cast<Dynamic::Bytes&&>(*payload));
    members.insert(Member(
        selected.get_name(), dialect->get_name(),
        payloads[payloads.get_size() - 1].get_view()));
  }

  Managed::Vector<Package::Archive::Resource> resources(arena);
  for (const Tetrodotoxin::Source::Reference<Package::Resource>& retained :
       package.get_resources().get_values()) {
    const Package::Resource& resource = retained.get();
    resources.insert(
        Package::Archive::Resource(resource.get_route(), resource.get_value()));
  }

  Archive archive(
      identity, version, members.get_view(), resources.get_view(), imports);
  return write(archive);
}
