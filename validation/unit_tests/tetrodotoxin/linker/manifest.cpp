// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/linker/manifest.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/linker/provider.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Validation;

static constexpr U8 manifest_golden[] = {
  0x54, 0x54, 0x58, 0x41, 0x42, 0x49, 0x30, 0x31, 0x08, 0x00, 0x00, 0x00, 0x50,
  0x6B, 0x67, 0x2E, 0x43, 0x6F, 0x72, 0x65, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00,
  0x00, 0x00, 0x63, 0x70, 0x75, 0x11, 0x00, 0x00, 0x00, 0x78, 0x38, 0x36, 0x5F,
  0x36, 0x34, 0x2D, 0x73, 0x79, 0x73, 0x76, 0x2D, 0x6C, 0x69, 0x6E, 0x75, 0x78,
  0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x43, 0x0B, 0x00, 0x00, 0x00, 0x6E, 0x61, 0x74, 0x69,
  0x76, 0x65, 0x5F, 0x63, 0x61, 0x6C, 0x6C, 0x08, 0x00, 0x00, 0x00, 0x50, 0x6B,
  0x67, 0x2E, 0x48, 0x6F, 0x73, 0x74,
};

static auto golden() -> View::Bytes {
  return View::Bytes(manifest_golden);
}

static auto selected_manifest(
    Result<Linker::Manifest, Linker::Manifest::Error>& result)
    -> Linker::Manifest* {
  return result.visit(
      [](Linker::Manifest& manifest) { return &manifest; },
      [](Linker::Manifest::Error&) {
        return static_cast<Linker::Manifest*>(nullptr);
      });
}

static auto rejects(View::Bytes bytes, Linker::Manifest::Error expected)
    -> Bool {
  Allocator::Arena arena;
  auto result = Linker::Manifest::read(arena, bytes);
  return result.visit(
      [](const Linker::Manifest&) { return False; },
      [&](Linker::Manifest::Error error) {
        return error == expected ? True : False;
      });
}

static Harness LinkerManifest = {
  .name = "Tetrodotoxin::Linker::Manifest"_view,
};

PERIMORTEM_UNIT_TEST(LinkerManifest, canonical_round_trip) {
  Allocator::Arena arena;
  auto decoded = Linker::Manifest::read(arena, golden());
  auto manifest = selected_manifest(decoded);
  ASSERT(manifest);
  EXPECT_TEXT(manifest->get_identity(), "Pkg.Core"_view);
  EXPECT(manifest->get_version() == Perimortem::System::Version(1, 2));
  EXPECT_TEXT(manifest->get_artifact(), "cpu"_view);
  EXPECT_TEXT(manifest->get_target(), "x86_64-sysv-linux"_view);
  EXPECT_EQ(manifest->get_fingerprint().get_value(), U64(0x0123456789ABCDEF));
  ASSERT_EQ(manifest->get_imports().get_size(), Count(1));
  const Linker::Import& imported = manifest->get_imports().get_data()[0];
  EXPECT(imported.get_kind() == Linker::Import::Kind::Function);
  EXPECT_TEXT(imported.get_abi(), "C"_view);
  EXPECT_TEXT(imported.get_symbol(), "native_call"_view);
  EXPECT_TEXT(imported.get_provider(), "Pkg.Host"_view);

  auto encoded = Linker::Manifest::write(*manifest);
  ASSERT(encoded);
  EXPECT(encoded->get_view() == golden());
}

PERIMORTEM_UNIT_TEST(LinkerManifest, bounded_rejection) {
  for (Count size = 0; size < golden().get_size(); size++) {
    EXPECT(rejects(
        golden().slice(0, size), Linker::Manifest::Error::InvalidFormat));
  }

  Dynamic::Bytes bad_magic(golden());
  bad_magic.get_access().get_data()[0] = 'X';
  EXPECT(rejects(bad_magic, Linker::Manifest::Error::InvalidFormat));

  Dynamic::Bytes future(golden());
  future.get_access().get_data()[7] = '2';
  EXPECT(rejects(future, Linker::Manifest::Error::UnsupportedFormat));

  Dynamic::Bytes invalid_kind(golden());
  invalid_kind.get_access().get_data()[64] = 3;
  EXPECT(rejects(invalid_kind, Linker::Manifest::Error::InvalidFormat));
}

PERIMORTEM_UNIT_TEST(LinkerManifest, readable_fingerprint) {
  Allocator::Arena arena;
  Linker::Fingerprint fingerprint(0x0123456789ABCDEF);
  View::Bytes rendered = fingerprint.render(arena);
  EXPECT_TEXT(rendered, "0123456789abcdef"_view);
  auto parsed = Linker::Fingerprint::parse(rendered);
  ASSERT(parsed);
  EXPECT(*parsed == fingerprint);
  EXPECT_NOT(Linker::Fingerprint::parse("0123456789abcdeg"_view));
}

PERIMORTEM_UNIT_TEST(LinkerManifest, provider_selection) {
  Linker::Provider providers[] = {
    Linker::Provider(
        "Host.Linux"_view, "x86_64-sysv-linux"_view,
        Linker::Import::Kind::Function, "C"_view, "native_call"_view),
    Linker::Provider(
        "Host.Windows"_view, "x86_64-win64-windows"_view,
        Linker::Import::Kind::Function, "C"_view, "native_call"_view),
  };
  Linker::Import imported(
      Linker::Import::Kind::Function, "C"_view, "native_call"_view);
  auto selected =
      Linker::Provider::select(providers, "x86_64-sysv-linux"_view, imported);
  Bool selected_linux = selected.visit(
      [](const Linker::Import& value) {
        return value.get_provider() == "Host.Linux"_view ? True : False;
      },
      [](Linker::Provider::Error) { return False; });
  EXPECT(selected_linux);

  auto missing =
      Linker::Provider::select(providers, "aarch64-linux"_view, imported);
  EXPECT(missing.visit(
      [](const Linker::Import&) { return False; },
      [](Linker::Provider::Error error) {
        return error == Linker::Provider::Error::Missing ? True : False;
      }));

  Linker::Provider duplicate[] = {
    providers[0],
    Linker::Provider(
        "Host.Other"_view, "x86_64-sysv-linux"_view,
        Linker::Import::Kind::Function, "C"_view, "native_call"_view),
  };
  auto ambiguous =
      Linker::Provider::select(duplicate, "x86_64-sysv-linux"_view, imported);
  EXPECT(ambiguous.visit(
      [](const Linker::Import&) { return False; },
      [](Linker::Provider::Error error) {
        return error == Linker::Provider::Error::Ambiguous ? True : False;
      }));
}
