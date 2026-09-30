// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/dialects/source/formatter.hpp"

#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/dialects/source/tokenizer.hpp"
#include "toolchain/validation/unit_test.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Dialects::Source;
using namespace Toolchain::Validation;

static Harness formatter_tests("Source::Formatter");

static auto format(Stream stream, Test::TestResult& result) -> Dynamic::Bytes {
  return Formatter(stream).format().visit(
      [](Dynamic::Bytes text) { return text; },
      [&](Ttx::Semantic::Negotiation::Binding::Failure) {
        EXPECT(false);
        return Dynamic::Bytes();
      });
}

// Formatting groups declarations by their established source role while
// keeping attributes with their owner. The exact output checks ordering,
// documentation insertion and section boundaries together.
VALIDATION_TEST(formatter_tests, declaration_order) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "private Hidden:struct{public state value:U8;} "
      "private hidden:func=[self]->U8{return 1;} "
      "@abi(\"C\") @symbol(\"first\") "
      "public first:func=[]->U8{return 2;} "
      "using Core from dependency; dialect:Library; "
      "public state value:U8=0; "
      "public named:func=[self]->U8{return value;} "
      "private local:func=[]->U8{return 3;} "
      "public CountAlias:alias=U8; "
      "private const limit:U8=4; "
      "foreign \"C\"{public func imported_z[]->U8; "
      "public func imported_a[]->U8;} "
      "public Visible:enum[U8]{first=0;second=1;}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = format(tokenizer.get_stream(), result);
  EXPECT_TEXT(
      formatted,
      "//\n"
      "// Place holder source documentation.\n"
      "//\n"
      "\n"
      "dialect : Library;\n"
      "\n"
      "using Core from dependency;\n"
      "\n"
      "foreign \"C\" {\n"
      "  public func imported_a[] -> U8;\n"
      "  public func imported_z[] -> U8;\n"
      "}\n"
      "\n"
      "public CountAlias : alias = U8;\n"
      "\n"
      "private const limit : U8 = 4;\n"
      "\n"
      "public state value : U8 = 0;\n"
      "\n"
      "@abi(\"C\") @symbol(\"first\")\n"
      "public first : func = [] -> U8 : return 2;\n"
      "\n"
      "public named : func = [self] -> U8 : return value;\n"
      "\n"
      "private local : func = [] -> U8 : return 3;\n"
      "\n"
      "private hidden : func = [self] -> U8 : return 1;\n"
      "\n"
      "public Visible : enum[U8] {\n"
      "  first = 0;\n"
      "  second = 1;\n"
      "}\n"
      "\n"
      "private Hidden : struct {\n"
      "  public state value : U8;\n"
      "}\n"_view);
}

// Aliases that import sources or packages belong to the import group, ahead
// of ordinary aliases. Their request arguments must survive formatting
// without becoming declarations.
VALIDATION_TEST(formatter_tests, import_aliases) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library; public Zed:alias=source(\"z.ttx\"); "
      "public Alpha:alias=package(.name=\"Example\",.version=\"1.0\"); "
      "public Local:alias=U8;"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = format(tokenizer.get_stream(), result);
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "public Zed : alias = source(\"z.ttx\");\n"
      "public Alpha : alias = package(.name = \"Example\", .version = "
      "\"1.0\");\n"
      "\n"
      "public Local : alias = U8;\n"_view);
}

// Inferred declarations, prefix operators and postfix operators require
// different spacing. Combining them with compact control bodies checks that
// formatting preserves their syntactic attachment.
VALIDATION_TEST(formatter_tests, inferred_declaration) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library; public create:func=[]->U8{state value:=-1; "
      "if !false {value+=1;} for [.entry:U8] in 0...1 {value+=entry;} "
      "return value!+2;}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = format(tokenizer.get_stream(), result);
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "public create : func = [] -> U8 {\n"
      "  state value := -1;\n"
      "  if !false : value += 1;\n"
      "  for [.entry : U8] in 0...1 : value += entry;\n"
      "\n"
      "  return value! + 2;\n"
      "}\n"_view);
}

// Comparison and logical operators must remain inside the condition while
// signature packs retain their delimiters. This guards the boundary between
// a compressed body and its condition.
VALIDATION_TEST(formatter_tests, canonical_if_pack) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library; public check:func=[.first:S64,.second:S64,.third:S64]"
      "->[]{if first<second:first=second;"
      "if first<second and (second<third or third<first):return;}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = format(tokenizer.get_stream(), result);
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "public check : func = [.first : S64, .second : S64, .third : S64] -> "
      "[] {\n"
      "  if first < second : first = second;\n"
      "  if first < second and (second < third or third < first) : return;\n"
      "}\n"_view);
}

// A composite formats its own constants, fields, callables and nested types
// as separate groups. Its member ordering must not be inherited from the
// surrounding source scope.
VALIDATION_TEST(formatter_tests, composite_order) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library; public Container:struct{private Nested:struct{} "
      "private self_call:func=[self]->[]{return;} "
      "public state first:U8; private const beta:U8=2; "
      "public static_call:func=[]->[]{return;} "
      "private state second:U8; public const alpha:U8=1; "
      "public self_call:func=[self]->[]{return;} "
      "public Nested:enum[U8]{first=0;} "
      "private static_call:func=[]->[]{return;}}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = format(tokenizer.get_stream(), result);
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "public Container : struct {\n"
      "  public const alpha : U8 = 1;\n"
      "  private const beta : U8 = 2;\n"
      "\n"
      "  public state first   : U8;\n"
      "  private state second : U8;\n"
      "\n"
      "  public static_call : func = [] -> [] : return;\n"
      "\n"
      "  public self_call : func = [self] -> [] : return;\n"
      "\n"
      "  private static_call : func = [] -> [] : return;\n"
      "\n"
      "  private self_call : func = [self] -> [] : return;\n"
      "\n"
      "  public Nested : enum[U8] {\n"
      "    first = 0;\n"
      "  }\n"
      "\n"
      "  private Nested : struct {}\n"
      "}\n"_view);
}

// Names sort within the appropriate visibility and declaration group.
// Repeated names across groups expose a sorter that merges distinct
// sections or loses their boundaries.
VALIDATION_TEST(formatter_tests, alphabetical_blocks) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library; private ZebraAlias:alias=U8; "
      "public ZebraAlias:alias=U8; "
      "private AlphaAlias:alias=U8; "
      "public AlphaAlias:alias=U8; "
      "private const zebra:U8=2; "
      "public const alpha:U8=1; "
      "public zebra:func=[]->[]{return;} "
      "public alpha:func=[]->[]{return;} "
      "private Zebra:struct{} public Zebra:struct{} "
      "private Alpha:struct{} public Alpha:struct{}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = format(tokenizer.get_stream(), result);
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "public AlphaAlias  : alias = U8;\n"
      "public ZebraAlias  : alias = U8;\n"
      "private AlphaAlias : alias = U8;\n"
      "private ZebraAlias : alias = U8;\n"
      "\n"
      "public const alpha  : U8 = 1;\n"
      "private const zebra : U8 = 2;\n"
      "\n"
      "public alpha : func = [] -> [] : return;\n"
      "public zebra : func = [] -> [] : return;\n"
      "\n"
      "public Alpha : struct {}\n"
      "\n"
      "public Zebra : struct {}\n"
      "\n"
      "private Alpha : struct {}\n"
      "\n"
      "private Zebra : struct {}\n"_view);
}

// Documentation stays with the declaration or statement it describes,
// including intervening attributes. Empty authored documentation remains
// visible rather than disappearing during paragraph normalization.
VALIDATION_TEST(formatter_tests, documentation_break) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "//Source.\n"
      "dialect:Library; @abi(\"C\") // Function documentation.\n"
      "@symbol(\"run\") public run:func=[]->[]{state first:U8=0; "
      "first=1; //Observed statement.\n"
      "first; //\n"
      "return;}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = format(tokenizer.get_stream(), result);
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "// Function documentation.\n"
      "@abi(\"C\") @symbol(\"run\")\n"
      "public run : func = [] -> [] {\n"
      "  state first : U8 = 0;\n"
      "  first = 1;\n"
      "\n"
      "  // Observed statement.\n"
      "  first;\n"
      "  //\n"
      "  return;\n"
      "}\n"_view);
}

// Raw comment bytes remain unchanged while ordinary documentation receives
// spacing normalization. Metadata must not be rewritten through the public
// prose formatting path.
VALIDATION_TEST(formatter_tests, raw_comments) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "///Tetrodotoxin   metadata\n"
      "///  Copyright metadata\n"
      "//Public documentation.\n"
      "dialect:Library;"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = format(tokenizer.get_stream(), result);
  EXPECT_TEXT(
      formatted,
      "///Tetrodotoxin   metadata\n"
      "///  Copyright metadata\n"
      "// Public documentation.\n"
      "dialect : Library;\n"_view);
}

// Hexadecimal integers receive canonical width and casing, and byte
// literals receive canonical byte separation. The exact output
// distinguishes their separate formatting rules.
VALIDATION_TEST(formatter_tests, hexadecimal_literals) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "//Source.\n"
      "dialect:Library; private const small:=0xa; "
      "private const medium:=0xabc; private const large:=0xabcde; "
      "private const bytes:=0x[0a   bC\t00];"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = format(tokenizer.get_stream(), result);
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "private const bytes  := 0x[0A BC 00];\n"
      "private const large  := 0x000ABCDE;\n"
      "private const medium := 0x0ABC;\n"
      "private const small  := 0x0A;\n"_view);
}

// Alignment stays within a small declaration group and does not pad
// unrelated assignments. A long name must break alignment rather than force
// distant columns across the source.
VALIDATION_TEST(formatter_tests, alignment_islands) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library; private const a:U64=1; "
      "private const extraordinarily_long_constant_name:U64=2; "
      "public Table:struct{public state count:U64=0; "
      "private state selected:Bool=false;} "
      "public assign:func=[]->[]{first=1;longer_name=2;return;}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = format(tokenizer.get_stream(), result);
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "private const a : U64 = 1;\n"
      "private const extraordinarily_long_constant_name : U64 = 2;\n"
      "\n"
      "public assign : func = [] -> [] {\n"
      "  first = 1;\n"
      "  longer_name = 2;\n"
      "}\n"
      "\n"
      "public Table : struct {\n"
      "  public state count     : U64  = 0;\n"
      "  private state selected : Bool = false;\n"
      "}\n"_view);
}

// Each multiline argument pack owns its indentation and alignment. Adjacent
// assignments with long receivers must not spread one pack layout into the
// next.
VALIDATION_TEST(formatter_tests, pack_alignment_is_local) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library;public update:func=[]->[]{"
      "self.icon_top_shader.parameters.tone=(.x=1,.y=2,);"
      "self.icon_bottom_shader.parameters.tone=(.x=3,.y=4,);return;}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = format(tokenizer.get_stream(), result);
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "public update : func = [] -> [] {\n"
      "  self.icon_top_shader.parameters.tone = (\n"
      "    .x = 1,\n"
      "    .y = 2,\n"
      "  );\n"
      "  self.icon_bottom_shader.parameters.tone = (\n"
      "    .x = 3,\n"
      "    .y = 4,\n"
      "  );\n"
      "}\n"_view);
}

// An authored trailing comma or an oversized pack selects multiline layout.
// Entry order remains authored even while indentation and separators become
// canonical.
VALIDATION_TEST(formatter_tests, canonical_pack_width) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library; private const compact:=(.right=2,.left=1,); "
      "private const expanded:=(.first=11111111111111111111,"
      ".second=22222222222222222222,.third=33333333333333333333);"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = format(tokenizer.get_stream(), result);
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "private const compact  := (\n"
      "  .right = 2,\n"
      "  .left = 1,\n"
      ");\n"
      "private const expanded := (\n"
      "  .first = 11111111111111111111,\n"
      "  .second = 22222222222222222222,\n"
      "  .third = 33333333333333333333,\n"
      ");\n"_view);
}

// A trailing comma expands a braced value list into separate lines. The
// formatter must recognize this form without treating its members as a
// block of declarations.
VALIDATION_TEST(formatter_tests, braced_trailing_comma) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:App;runtime=Windowed{.title=\"Example\",.width=800,"
      ".height=600,.resizable=true,} lifecycle{initial Main;}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = format(tokenizer.get_stream(), result);
  EXPECT(
      Algorithm::search(
          formatted.get_view(),
          "runtime = Windowed {\n"
          "  .title = \"Example\",\n"
          "  .width = 800,\n"
          "  .height = 600,\n"
          "  .resizable = true,\n"
          "}"_view) != Count(-1));
}

// Grouping parentheses in a condition are not a value pack. Wrapping the
// enclosing signature must not insert a comma into that expression.
VALIDATION_TEST(formatter_tests, grouped_expression) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library;public check:func=[.first:S64,.second:S64,.third:S64,"
      ".fourth:S64]->Bool:return first<second and (second<third or third<fourth);"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = format(tokenizer.get_stream(), result);
  EXPECT(
      Algorithm::search(formatted.get_view(), "third < fourth,"_view) ==
      Count(-1));
}

// A long signature may wrap while its empty result remains one empty pair
// of brackets. Width handling must not turn that result into a multiline
// argument list.
VALIDATION_TEST(formatter_tests, empty_result_wrapping) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library; public set_extraordinarily_long_transform_name:func="
      "[self,.transform:Transform2D]->[]:self.transform=transform;"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = format(tokenizer.get_stream(), result);
  EXPECT(Algorithm::search(formatted.get_view(), "[]"_view) != Count(-1));
  EXPECT(Algorithm::search(formatted.get_view(), "-> [\n"_view) == Count(-1));
}

// Empty result bodies can normalize trailing returns without reordering
// effects. Returns before later statements remain visible, preserving
// malformed authored control flow for inspection.
VALIDATION_TEST(formatter_tests, empty_fallthrough) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library; public empty:func=[]->[]{} "
      "public effect:func=[]->[]{value=1;return;} "
      "public many:func=[]->[]{first=1;second=2;return;} "
      "public explicit:func=[]->[]:return; "
      "public malformed:func=[]->[]{return;value=1;} "
      "public repeated:func=[]->[]{return;return;}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = format(tokenizer.get_stream(), result);
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "public effect : func = [] -> [] : value = 1;\n"
      "public empty : func = [] -> [] : return;\n"
      "public explicit : func = [] -> [] : return;\n"
      "public malformed : func = [] -> [] {\n"
      "  return;\n"
      "  value = 1;\n"
      "}\n"
      "\n"
      "public many : func = [] -> [] {\n"
      "  first = 1;\n"
      "  second = 2;\n"
      "}\n"
      "\n"
      "public repeated : func = [] -> [] : return;\n"_view);
}

// Executable paragraphs group declarations with their following work and
// separate the next declaration. Compact control bodies must not introduce
// unrelated blank lines.
VALIDATION_TEST(formatter_tests, executable_spacing) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library; public run:func=[]->[]{state first:U8=0; "
      "first=1; const second:U8=2; second; if true {first=2;} "
      "while false {break;} return;}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = format(tokenizer.get_stream(), result);
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "public run : func = [] -> [] {\n"
      "  state first : U8 = 0;\n"
      "  first = 1;\n"
      "\n"
      "  const second : U8 = 2;\n"
      "  second;\n"
      "  if true : first = 2;\n"
      "  while false : break;\n"
      "}\n"_view);
}

// Call arrows receive canonical spacing at every nesting level. Receiver
// access and the argument list must stay attached to the same call.
VALIDATION_TEST(formatter_tests, call_spacing) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library; public run:func=[]->[]{"
      "state output:=Dynamic::Bytes->copy(\"value\"->get_view());"
      "output  ->  clear();}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = format(tokenizer.get_stream(), result);
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "public run : func = [] -> [] {\n"
      "  state output := Dynamic::Bytes -> copy(\"value\" -> get_view());\n"
      "  output -> clear();\n"
      "}\n"_view);
}

// A stage qualifier remains separated from its signature pack. The
// formatter must not apply ordinary indexing spacing to this declaration
// form.
VALIDATION_TEST(formatter_tests, qualifier_spacing) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena, "// Source.\ndialect:Pipeline;public fragment:stage[]->[];"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = format(tokenizer.get_stream(), result);
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Pipeline;\n"
      "\n"
      "public fragment : stage [] -> [];\n"_view);
}

// Shader stages use their established stage order after the implements
// clause. The exact output protects this source formatting behavior
// independently of executable Shader semantics.
VALIDATION_TEST(formatter_tests, shader_stage_order) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Shader;implements source(\"pipeline.ttx\");"
      "Shader fragment[]->[]{return;}Shader vertex[]->[]{return;}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = format(tokenizer.get_stream(), result);
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Shader;\n"
      "\n"
      "implements source(\"pipeline.ttx\");\n"
      "\n"
      "Shader vertex[] -> [] : return;\n"
      "Shader fragment[] -> [] : return;\n"_view);
}

// Adjacent compressed control bodies share a paragraph, while work
// following them starts a new one. A full block retains the stronger
// paragraph boundary.
VALIDATION_TEST(formatter_tests, compressed_spacing) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library; public run:func=[]->[]{state alpha:U64=0;"
      "alpha=0;if first:alpha=1;if second:alpha=2;alpha=3;alpha=4;"
      "while third{alpha=5;alpha=6;}alpha=7;}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = format(tokenizer.get_stream(), result);
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "public run : func = [] -> [] {\n"
      "  state alpha : U64 = 0;\n"
      "  alpha = 0;\n"
      "  if first : alpha = 1;\n"
      "  if second : alpha = 2;\n"
      "\n"
      "  alpha = 3;\n"
      "  alpha = 4;\n"
      "  while third {\n"
      "    alpha = 5;\n"
      "    alpha = 6;\n"
      "  }\n"
      "\n"
      "  alpha = 7;\n"
      "}\n"_view);
}

// A single statement body may compress, but nested control flow must retain
// enough braces to keep its owner clear. Match cases exercise both compact
// and full bodies in the same source.
VALIDATION_TEST(formatter_tests, compact_blocks) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library; public run:func=[.first:Bool,.second:Bool]->[]{"
      "if first {if second {first;}} else {second;} match first {"
      "case true {first;} case _ {first;second;}} return;}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = format(tokenizer.get_stream(), result);
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "public run : func = [.first : Bool, .second : Bool] -> [] {\n"
      "  if first {\n"
      "    if second : first;\n"
      "  } else : second;\n"
      "\n"
      "  match first {\n"
      "    case true : first;\n"
      "    case _ {\n"
      "      first;\n"
      "      second;\n"
      "    }\n"
      "  }\n"
      "}\n"_view);
}

// Top level callables may reorder by name while statements inside each body
// preserve authored order. Formatting a later declaration cannot move it
// ahead of an earlier side effect.
VALIDATION_TEST(formatter_tests, callable_body_order) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Scene; Scene update[self]->Void{self.value=1; "
      "const observed:U8=2; return;} "
      "Scene release[self]->Void{return;}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = format(tokenizer.get_stream(), result);
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Scene;\n"
      "\n"
      "Scene release[self] -> Void : return;\n"
      "Scene update[self] -> Void {\n"
      "  self.value = 1;\n"
      "\n"
      "  const observed : U8 = 2;\n"
      "  return;\n"
      "}\n"_view);
}

// Formatting incomplete source must retain its unrecognized bytes. A second
// pass must produce the same output so editor formatting does not keep
// changing broken input.
VALIDATION_TEST(formatter_tests, stable_malformed) {
  Allocator::Arena first_arena;
  Tokenizer first(
      first_arena,
      "// Source.\n"
      "dialect:Library; public state value:U8 = ^ "_view,
      "Test.ttx"_view);
  Dynamic::Bytes first_output = format(first.get_stream(), result);

  Allocator::Arena second_arena;
  Tokenizer second(second_arena, first_output.get_view(), "Test.ttx"_view);
  Dynamic::Bytes second_output = format(second.get_stream(), result);

  EXPECT_TEXT(first_output, second_output.get_view());
  EXPECT_TEXT(
      first_output,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "public state value : U8 = ^\n"_view);
}

// A long byte literal followed by a slice keeps the postfix operation
// attached to its receiver. Wrapping within the index expression must
// preserve the remaining operands and delimiters.
VALIDATION_TEST(formatter_tests, postfix_wrapping) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library; private value:Fixed[U8,2]="
      "0x[00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F]"
      ":[Parameters.offset+12,Parameters.size];"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = format(tokenizer.get_stream(), result);
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "private value : Fixed[U8, 2] = "
      "0x[00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F]"
      ":[Parameters.offset +\n"
      "    12, Parameters.size];\n"_view);
}
