// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/repository/repository.hpp"

#include "validation/unit_test.hpp"

#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/system/version.hpp"

#include "tetrodotoxin/package/archive/writer.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin;
using namespace Validation;

static Harness PackageRepository = {
  .name = "Package repository"_view,
};

// Repository selection consumes archive bytes. Constructing those bytes here
// keeps the test independent of whichever Dialect Puffer bootstraps.
class RepositoryFixture {
 public:
  RepositoryFixture() : created(mkdtemp(root) != nullptr) {}
  ~RepositoryFixture() {
    if (!created) {
      return;
    }
    unlink(path("/Example.Package/1.0/package.ttxp"_view));
    rmdir(path("/Example.Package/1.0"_view));
    rmdir(path("/Example.Package"_view));
    rmdir(root);
  }
  auto get_root() const -> Core::View::Bytes {
    return Core::NullTerminated::to_view(root);
  }
  auto write() -> Bool {
    if (!created) {
      return False;
    }
    if (mkdir(path("/Example.Package"_view), 0700) != 0) {
      return False;
    }
    if (mkdir(path("/Example.Package/1.0"_view), 0700) != 0) {
      return False;
    }
    const Package::Archive::Member member(
        "PackageSurface"_view, "Library"_view, "fixture"_view);
    const Package::Archive::Archive archive(
        "Example.Package"_view, System::Version(1, 0), {&member, 1});
    auto bytes = Package::Archive::Writer::write(archive);
    if (!bytes) {
      return False;
    }
    return System::File::write(
        *bytes, Core::NullTerminated::to_view(
                    path("/Example.Package/1.0/package.ttxp"_view)));
  }

 private:
  auto path(Core::View::Bytes suffix) -> const char* {
    Memory::Managed::Bytes value(arena, get_root());
    value.concat(suffix);
    value.append(0);
    return reinterpret_cast<const char*>(value.get_view().get_data());
  }
  Memory::Allocator::Arena arena;
  char root[64] = "/tmp/ttx-repository-XXXXXX";
  Bool created;
};

PERIMORTEM_UNIT_TEST(PackageRepository, exact_complete_product) {
  Memory::Allocator::Arena arena;
  RepositoryFixture fixture;
  ASSERT(fixture.write());
  auto repository =
      Package::Repository::Repository::create(arena, fixture.get_root());
  ASSERT(repository);

  const Package::Archive::Archive* first = nullptr;
  repository->select_archive("Example.Package"_view, System::Version(1, 0))
      .visit(
          [&](const Package::Archive::Archive& archive) { first = &archive; },
          [](Package::Repository::Repository::Error) {});
  ASSERT(first);
  EXPECT_TEXT(first->get_identity(), "Example.Package"_view);
  EXPECT(first->get_version() == System::Version(1, 0));
  const Package::Archive::Archive* repeated = nullptr;
  repository->select_archive("Example.Package"_view, System::Version(1, 0))
      .visit(
          [&](const Package::Archive::Archive& archive) {
            repeated = &archive;
          },
          [](Package::Repository::Repository::Error) {});
  ASSERT(repeated);
  EXPECT_TEXT(repeated->get_identity(), first->get_identity());
  EXPECT(repeated->get_version() == first->get_version());
}

PERIMORTEM_UNIT_TEST(PackageRepository, missing_coordinate) {
  Memory::Allocator::Arena arena;
  RepositoryFixture fixture;
  ASSERT(fixture.write());
  auto repository =
      Package::Repository::Repository::create(arena, fixture.get_root());
  ASSERT(repository);

  Bool missing =
      repository
          ->select_archive("Perimortem.Missing"_view, System::Version(1, 0))
          .visit(
              [](const Package::Archive::Archive&) -> Bool { return False; },
              [](Package::Repository::Repository::Error error) -> Bool {
                return error == Package::Repository::Repository::Error::
                                    NotDeclared
                           ? True
                           : False;
              });
  EXPECT(missing);
}
