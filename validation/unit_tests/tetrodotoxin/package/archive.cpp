// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/archive/archive.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/package/archive/reader.hpp"
#include "tetrodotoxin/package/archive/writer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Validation;

static Harness PackageArchive = {
  .name = "Tetrodotoxin::Package::Archive"_view,
};

static auto select_archive(
    Result<Package::Archive::Archive, Package::Archive::Reader::Error>& result)
    -> Option<Package::Archive::Archive&> {
  return result.visit(
      [](Package::Archive::Archive& archive)
          -> Option<Package::Archive::Archive&> { return archive; },
      [](Package::Archive::Reader::Error)
          -> Option<Package::Archive::Archive&> { return {}; });
}

static auto has_error(
    Result<Package::Archive::Archive, Package::Archive::Reader::Error>& result,
    Package::Archive::Reader::Error expected) -> Bool {
  return result.visit(
      [](Package::Archive::Archive&) { return False; },
      [&](Package::Archive::Reader::Error error) {
        return error == expected ? True : False;
      });
}

static auto set_u16(Dynamic::Bytes& bytes, Count offset, U16 value) -> void {
  auto data = bytes.get_access();
  data.get_data()[offset] = U8(value);
  data.get_data()[offset + 1] = U8(value >> 8);
}

static auto rejected(const Package::Archive::Archive& archive) -> Bool {
  auto encoded = Package::Archive::Writer::write(archive);
  BAIL_IF(!encoded);
  Allocator::Arena arena;
  auto decoded = Package::Archive::Reader::read(arena, *encoded);
  return has_error(decoded, Package::Archive::Reader::Error::InvalidFormat);
}

PERIMORTEM_UNIT_TEST(PackageArchive, complete_graph_round_trip) {
  Package::Archive::Member members[] = {
    Package::Archive::Member(
        "PackageSurface"_view, "Library"_view, "package"_view),
    Package::Archive::Member("Main"_view, "Library"_view, "member"_view),
  };
  Package::Archive::Resource resources[] = {
    Package::Archive::Resource("resources/data.bin"_view, "payload"_view),
  };
  Package::Archive::GraphImport imports[] = {
    Package::Archive::GraphImport(
        "PackageSurface"_view, "Shared"_view, Language::Visibility::Public,
        Language::Import::Kind::Source, "Main"_view, {}, "Root::Type"_view),
    Package::Archive::GraphImport(
        "Main"_view, "Core"_view, Language::Visibility::Private,
        Language::Import::Kind::Package, "Perimortem.Core"_view, Version(1, 2)),
  };
  Package::Archive::Archive archive(
      "Example.Package"_view, Version(3, 4), members, resources, imports);

  auto encoded = Package::Archive::Writer::write(archive);
  ASSERT(encoded);
  ASSERT(encoded->get_size() > Package::Archive::Archive::header_size);
  EXPECT_EQ(encoded->get_view()[4], U8(Package::Archive::Archive::format));
  EXPECT_EQ(encoded->get_view()[5], U8(0));

  Allocator::Arena arena;
  auto decoded = Package::Archive::Reader::read(arena, *encoded);
  auto selected = select_archive(decoded);
  ASSERT(selected);
  EXPECT_TEXT(selected->get_identity(), "Example.Package"_view);
  EXPECT(selected->get_version() == Version(3, 4));
  ASSERT_EQ(selected->get_members().get_size(), Count(2));
  EXPECT_TEXT(
      selected->get_members().get_data()[0].get_semantic_name(),
      "PackageSurface"_view);
  EXPECT_TEXT(
      selected->get_members().get_data()[1].get_payload(), "member"_view);
  ASSERT_EQ(selected->get_resources().get_size(), Count(1));
  EXPECT_TEXT(
      selected->get_resources().get_data()[0].get_value(), "payload"_view);
  ASSERT_EQ(selected->get_imports().get_size(), Count(2));
  EXPECT(
      selected->get_imports().get_data()[1].get_kind() ==
      Language::Import::Kind::Package);
  EXPECT(
      selected->get_imports().get_data()[1].get_visibility() ==
      Language::Visibility::Private);

  auto repeated = Package::Archive::Writer::write(*selected);
  ASSERT(repeated);
  EXPECT(repeated->get_view() == encoded->get_view());
}

PERIMORTEM_UNIT_TEST(PackageArchive, exact_envelope) {
  Package::Archive::Member member("PackageSurface"_view, "Library"_view, {});
  Package::Archive::Archive archive(
      "Example.Package"_view, Version(1, 0), View::Vector(&member, 1));
  auto encoded = Package::Archive::Writer::write(archive);
  ASSERT(encoded);

  Dynamic::Bytes invalid_magic(encoded->get_view());
  invalid_magic.get_access().get_data()[0] = 'X';
  Allocator::Arena magic_arena;
  auto magic = Package::Archive::Reader::read(magic_arena, invalid_magic);
  EXPECT(has_error(magic, Package::Archive::Reader::Error::InvalidFormat));

  Dynamic::Bytes unsupported(encoded->get_view());
  set_u16(unsupported, 4, Package::Archive::Archive::format + 1);
  Allocator::Arena version_arena;
  auto version = Package::Archive::Reader::read(version_arena, unsupported);
  EXPECT(
      has_error(version, Package::Archive::Reader::Error::UnsupportedFormat));

  Dynamic::Bytes flags(encoded->get_view());
  set_u16(flags, 6, 1);
  Allocator::Arena flags_arena;
  auto flagged = Package::Archive::Reader::read(flags_arena, flags);
  EXPECT(has_error(flagged, Package::Archive::Reader::Error::InvalidFormat));

  Dynamic::Bytes trailing(encoded->get_view());
  trailing.append(0);
  Allocator::Arena trailing_arena;
  auto extra = Package::Archive::Reader::read(trailing_arena, trailing);
  EXPECT(has_error(extra, Package::Archive::Reader::Error::InvalidFormat));

  Allocator::Arena truncated_arena;
  auto truncated = Package::Archive::Reader::read(
      truncated_arena, encoded->get_view().slice(0, encoded->get_size() - 1));
  EXPECT(has_error(truncated, Package::Archive::Reader::Error::InvalidFormat));
}

PERIMORTEM_UNIT_TEST(PackageArchive, rejects_non_graph_facts) {
  Package::Archive::Member surface("PackageSurface"_view, "Library"_view, {});
  Package::Archive::Member duplicate_members[] = {surface, surface};
  Package::Archive::Archive duplicate(
      "Example.Package"_view, Version(1, 0), duplicate_members);
  EXPECT(rejected(duplicate));

  Package::Archive::Resource escaping("../escape"_view, {});
  Package::Archive::Archive escaped(
      "Example.Package"_view, Version(1, 0), View::Vector(&surface, 1),
      View::Vector(&escaping, 1));
  EXPECT(rejected(escaped));

  Package::Archive::GraphImport missing(
      "Missing"_view, "Core"_view, Language::Import::Kind::Package,
      "Perimortem.Core"_view, Version(1, 0));
  Package::Archive::Archive unresolved(
      "Example.Package"_view, Version(1, 0), View::Vector(&surface, 1), {},
      View::Vector(&missing, 1));
  EXPECT(rejected(unresolved));

  Package::Archive::Archive no_coordinate(
      "Example.Package"_view, {}, View::Vector(&surface, 1));
  EXPECT(rejected(no_coordinate));
}
