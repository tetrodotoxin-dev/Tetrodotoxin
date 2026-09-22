// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/environment/toolchain.hpp"

#include "tetrodotoxin/source/documentation.hpp"

#include "validation/unit_test.hpp"

#include <cstdlib>
#include <unistd.h>

#include "perimortem/core/null_terminated.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/build/dialect.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Source;
using namespace Tetrodotoxin::Source::Lexical;
using namespace Validation;

class SourceFile {
 public:
  explicit SourceFile(View::Bytes text) {
    const int descriptor = mkstemp(path);
    if (descriptor < 0) {
      return;
    }
    close(descriptor);
    ready = Perimortem::System::File::write(text, get_path());
  }
  ~SourceFile() { unlink(path); }
  auto get_path() const -> View::Bytes { return NullTerminated::to_view(path); }
  Bool ready = False;

 private:
  char path[64] = "/tmp/ttx-toolchain-XXXXXX";
};

struct Lifetime {
  Count live_dialects = 0;
  Count destroyed_sources = 0;
  Bool released_before_dialect = False;
};

class ProcessedSource : public Language::Monograph {
 public:
  ProcessedSource(
      Allocator::Arena& arena,
      Language::Dialect& dialect,
      const Tetrodotoxin::Source::Documentation& documentation,
      Abstract& context,
      View::Bytes body,
      Lifetime& lifetime)
      : Monograph(arena, dialect, documentation, context),
        body(body),
        lifetime(lifetime) {}
  ~ProcessedSource() override {
    lifetime.destroyed_sources++;
    lifetime.released_before_dialect = lifetime.live_dialects > 0;
  }
  TTX_NAME("Processed"_view);
  auto link(Cursor&) -> Bool override {
    linked = True;
    return True;
  }
  View::Bytes body;
  Bool linked = False;

 private:
  Lifetime& lifetime;
};

class ProbeDialect : public Language::Dialect {
 public:
  explicit ProbeDialect(Lifetime& lifetime) : lifetime(lifetime) {
    lifetime.live_dialects++;
  }
  ~ProbeDialect() override { lifetime.live_dialects--; }
  TTX_NAME("Probe"_view);
  auto interpret(
      Cursor& cursor,
      const Tetrodotoxin::Source::Documentation& documentation,
      const Anchor&,
      Abstract& context) -> Option<Language::Monograph&> override {
    auto body = cursor.get_source_text().slice(cursor.current().get_offset());
    if (body == "reject"_view) {
      return {};
    }
    if (body == "partial"_view) {
      cursor.create_error("Probe retained a partial source."_view);
    }
    source = cursor.get_arena().construct<ProcessedSource>(
        cursor.get_arena(), *this, documentation, context, body, lifetime);
    return *source;
  }
  Option<ProcessedSource&> source;

 private:
  Lifetime& lifetime;
};

static Harness EnvironmentToolchain = {
  .name = "Tetrodotoxin::Environment::Toolchain"_view};

PERIMORTEM_UNIT_TEST(EnvironmentToolchain, registers_provided_names) {
  Lifetime lifetime;
  ProbeDialect dialect(lifetime);
  ProbeDialect duplicate(lifetime);
  Environment::Toolchain toolchain;
  EXPECT(toolchain.install(dialect));
  auto selected = toolchain.find("Probe"_view);
  ASSERT(selected);
  EXPECT(&*selected == &dialect);
  EXPECT_NOT(toolchain.install(duplicate));
  EXPECT_NOT(toolchain.find("Renamed"_view));
}

PERIMORTEM_UNIT_TEST(EnvironmentToolchain, borrows_dialects) {
  Lifetime lifetime;
  {
    ProbeDialect dialect(lifetime);
    {
      Environment::Toolchain toolchain;
      ASSERT(toolchain.install(dialect));
      Errors errors;
      SourceFile file("// Source\ndialect : Probe;\nbody"_view);
      ASSERT(file.ready);
      ASSERT(toolchain.process(file.get_path(), errors));
    }
    EXPECT_EQ(lifetime.live_dialects, Count(1));
    EXPECT_EQ(lifetime.destroyed_sources, Count(1));
    EXPECT(lifetime.released_before_dialect);
  }
  EXPECT_EQ(lifetime.live_dialects, Count(0));
}

PERIMORTEM_UNIT_TEST(EnvironmentToolchain, processes_source_body) {
  Lifetime lifetime;
  ProbeDialect dialect(lifetime);
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install(dialect));
  Errors errors;
  {
    SourceFile file(
        "// Source documentation\ndialect : Probe;\nprivate Input : alias = source(\"unused.ttx\");\n"_view);
    ASSERT(file.ready);
    ASSERT(toolchain.process(file.get_path(), errors));
  }
  // The file and its caller path have ended, but the returned source still
  // borrows valid bytes. The command proves dispatch stopped after the header.
  ASSERT(dialect.source);
  EXPECT_TEXT(
      dialect.source->body,
      "private Input : alias = source(\"unused.ttx\");\n"_view);
  EXPECT_NOT(dialect.source->linked);
  EXPECT(errors.is_empty());
}

PERIMORTEM_UNIT_TEST(EnvironmentToolchain, reports_source_failures) {
  Lifetime lifetime;
  ProbeDialect dialect(lifetime);
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install(dialect));
  const View::Bytes invalid[] = {
    "// Source\ndialect : Missing;\n"_view,
    "// Source\ndialect Probe;\n"_view,
    "// Source\ndialect : Probe;\nreject"_view,
    "dialect : Probe;\n"_view,
  };
  for (auto source : invalid) {
    SourceFile file(source);
    ASSERT(file.ready);
    Errors errors;
    EXPECT_NOT(toolchain.process(file.get_path(), errors));
    EXPECT_EQ(errors.get_size(), Count(1));
    EXPECT_TEXT(errors.get_source_name(0), file.get_path());
  }
  Errors unreadable;
  EXPECT_NOT(toolchain.process("/dev/null/missing.ttx"_view, unreadable));
  EXPECT_EQ(unreadable.get_size(), Count(1));
  EXPECT_TEXT(unreadable.get_source_name(0), "/dev/null/missing.ttx"_view);
}

PERIMORTEM_UNIT_TEST(EnvironmentToolchain, accumulates_partial_diagnostics) {
  Lifetime lifetime;
  ProbeDialect dialect(lifetime);
  Environment::Toolchain toolchain;
  ASSERT(toolchain.install(dialect));
  SourceFile partial("// Source\ndialect : Probe;\npartial"_view);
  SourceFile complete("// Source\ndialect : Probe;\ncomplete"_view);
  ASSERT(partial.ready && complete.ready);
  Errors errors;
  EXPECT(toolchain.process(partial.get_path(), errors));
  EXPECT_EQ(errors.get_size(), Count(1));
  EXPECT_TEXT(errors.get_message(0), "Probe retained a partial source."_view);
  EXPECT(toolchain.process(complete.get_path(), errors));
  EXPECT_EQ(errors.get_size(), Count(1));
}

PERIMORTEM_UNIT_TEST(EnvironmentToolchain, supplies_build_arguments) {
  const View::Bytes arguments[] = {
    "-help"_view, "-output=a path"_view, ""_view, "-choice=first"_view,
    "-choice=second"_view};
  Build::Dialect build(arguments);
  auto supplied = build.get_arguments();
  ASSERT_EQ(supplied.get_size(), Count(5));
  for (Count index = 0; index < supplied.get_size(); index++) {
    EXPECT_TEXT(supplied[index], arguments[index]);
  }
  EXPECT_TEXT(build.get_name(), "Build"_view);
}
