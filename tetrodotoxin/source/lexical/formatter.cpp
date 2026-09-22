// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/source/lexical/formatter.hpp"

#include "perimortem/core/math.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/source/lexical/lexicon.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Source::Lexical;

enum class Section : U8 {
  Header,
  Import,
  Foreign,
  PublicAlias,
  PrivateAlias,
  Constant,
  StaticVariable,
  InstanceVariable,
  PublicStaticFunction,
  PublicSelfFunction,
  PrivateStaticFunction,
  PrivateSelfFunction,
  PublicType,
  PrivateType,
};

enum class ScopeRole : U8 {
  Executable,
  Static,
  Instance,
  Enumeration,
};

enum class Shape : U8 {
  Definition,
  Statement,
  CompressedBlock,
  Block,
};

enum class AlignmentKind : U8 {
  None,
  Alias,
  Constant,
  StaticVariable,
  InstanceVariable,
  Local,
};

struct Unit {
  Count first;
  Count last;
  Count name;
  Section section;
  Shape shape;
  Bool documented;
};

struct Alignment {
  Count define = Count(-1);
  Count assign = Count(-1);
};

class State {
 public:
  State(const Tokenizer& tokenizer) : tokens(tokenizer) {}

  auto format() -> Dynamic::Bytes {
    Bool has_opening_documentation = False;
    for (Count index = 0;
         index < tokens.get_size() && tokens[index].get_code().is_comment();
         index++) {
      has_opening_documentation |=
          tokens[index].get_code() == Code::Type::Comment;
    }
    if (!has_opening_documentation) {
      output.concat("//\n// Place holder source documentation.\n//\n\n"_view);
    }

    write_units(0, content_end(), ScopeRole::Static);
    finish();
    return Data::take(output);
  }

 private:
  static constexpr Count line_limit = 100;

  auto content_end() const -> Count {
    Count end = tokens.get_size();

    if (end && tokens[end - 1].get_code() == Code::Type::Terminal) {
      end--;
    }

    return end;
  }

  auto find_pair(Count opening, Count end) const -> Count {
    Code::Type opening_code = tokens[opening].get_code().get_type();
    Code::Type closing_code = Code::Type::Terminal;

    if (opening_code == Code::Type::ScopeStart) {
      closing_code = Code::Type::ScopeEnd;
    } else if (opening_code == Code::Type::PackingStart) {
      closing_code = Code::Type::PackingEnd;
    } else if (opening_code == Code::Type::BracketStart) {
      closing_code = Code::Type::BracketEnd;
    } else {
      return end;
    }

    Count depth = 1;
    for (Count index = opening + 1; index < end; index++) {
      Code::Type code = tokens[index].get_code().get_type();

      if (code == opening_code) {
        depth++;
      } else if (code == closing_code) {
        depth--;
      }

      if (!depth) {
        return index;
      }
    }

    return end;
  }

  auto find_unit_end(Count first, Count end) const -> Count {
    Count scope_depth = 0;
    Count packing_depth = 0;
    Count bracket_depth = 0;
    for (Count index = first; index < end; index++) {
      Code::Type code = tokens[index].get_code().get_type();
      if (code == Code::Type::ScopeStart) {
        scope_depth++;
      } else if (code == Code::Type::ScopeEnd && scope_depth) {
        scope_depth--;
      } else if (code == Code::Type::PackingStart) {
        packing_depth++;
      } else if (code == Code::Type::PackingEnd && packing_depth) {
        packing_depth--;
      } else if (code == Code::Type::BracketStart) {
        bracket_depth++;
      } else if (code == Code::Type::BracketEnd && bracket_depth) {
        bracket_depth--;
      }

      Bool top_level = !scope_depth && !packing_depth && !bracket_depth;
      if (top_level && code == Code::Type::EndStatement) {
        Count next = index + 1;
        if (next < end && tokens[next].get_code() == Code::Type::Else) {
          continue;
        }

        return index + 1;
      }

      if (top_level && code == Code::Type::ScopeEnd) {
        Count next = index + 1;
        if (next < end && tokens[next].get_code() == Code::Type::Else) {
          continue;
        }

        return next;
      }
    }

    return end;
  }

  auto find_unit_start(Count first, Count last) const -> Count {
    Count current = first;

    while (current < last) {
      Code::Type code = tokens[current].get_code().get_type();

      if (tokens[current].get_code().is_comment()) {
        current++;
        continue;
      }

      if (code != Code::Type::Attribute) {
        break;
      }

      Count parameter = current + 1;
      if (parameter < last &&
          tokens[parameter].get_code() == Code::Type::PackingStart) {
        Count closing = find_pair(parameter, last);
        current = closing < last ? closing + 1 : last;
      } else {
        current++;
      }
    }

    return current;
  }

  auto classify(Count first, Count last, ScopeRole scope) const -> Section {
    Count unit_start = find_unit_start(first, last);
    Bool is_public = False;
    Bool is_self = False;
    Bool is_function = False;
    Bool is_alias = False;
    Bool is_type = False;
    Bool is_constant = False;
    Bool has_callable_arrow = False;
    Bool has_signature_layout = False;
    Bool has_assignment = False;
    Bool has_definition = False;
    Bool has_scope = False;

    for (Count index = unit_start; index < last; index++) {
      Code::Type code = tokens[index].get_code().get_type();

      if (code == Code::Type::Dialect) {
        return Section::Header;
      } else if (
          code == Code::Type::Using || code == Code::Type::Source ||
          code == Code::Type::Package) {
        return Section::Import;
      } else if (code == Code::Type::Public || code == Code::Type::Expose) {
        is_public = True;
      } else if (code == Code::Type::Func) {
        is_function = True;
      } else if (code == Code::Type::Self) {
        is_self = True;
      } else if (code == Code::Type::CallOp) {
        has_callable_arrow = True;
      } else if (
          code == Code::Type::BracketStart && !has_assignment &&
          !has_definition) {
        has_signature_layout = True;
      } else if (code == Code::Type::Assign) {
        has_assignment = True;
      } else if (code == Code::Type::Define) {
        has_definition = True;
      } else if (code == Code::Type::Alias) {
        is_alias = True;
      } else if (
          code == Code::Type::Struct || code == Code::Type::Object ||
          code == Code::Type::Interface || code == Code::Type::Implementation ||
          code == Code::Type::Namespace || code == Code::Type::Enum) {
        is_type = True;
      } else if (code == Code::Type::Const) {
        is_constant = True;
      }

      if (code == Code::Type::ScopeStart) {
        has_scope = True;
        break;
      }
    }

    if (unit_start < last && has_scope &&
        tokens[unit_start].get_code() == Code::Type::Addressable &&
        tokens.get_text(tokens[unit_start]) == "foreign"_view) {
      return Section::Foreign;
    }

    if (is_alias) {
      return is_public ? Section::PublicAlias : Section::PrivateAlias;
    }

    if (!is_function && has_callable_arrow && has_signature_layout) {
      is_function = True;
      is_public = True;
    }

    if (is_function) {
      if (is_public) {
        return is_self ? Section::PublicSelfFunction
                       : Section::PublicStaticFunction;
      }

      return is_self ? Section::PrivateSelfFunction
                     : Section::PrivateStaticFunction;
    }

    if (is_type) {
      return is_public ? Section::PublicType : Section::PrivateType;
    }

    if (is_constant || scope == ScopeRole::Enumeration) {
      return Section::Constant;
    }

    return scope == ScopeRole::Instance ? Section::InstanceVariable
                                        : Section::StaticVariable;
  }

  auto find_name(Count first, Count last) const -> Count {
    Count selected = find_unit_start(first, last);

    for (Count index = selected; index < last; index++) {
      Code::Type code = tokens[index].get_code().get_type();

      if (code == Code::Type::Define || code == Code::Type::Assign ||
          code == Code::Type::PackingStart ||
          code == Code::Type::BracketStart || code == Code::Type::ScopeStart ||
          code == Code::Type::EndStatement) {
        break;
      }

      if (code == Code::Type::Addressable || code == Code::Type::Type) {
        selected = index;
      }
    }

    return selected;
  }

  auto has_documentation(Count first, Count last) const -> Bool {
    Count content = find_unit_start(first, last);

    for (Count index = first; index < content; index++) {
      if (tokens[index].get_code() != Code::Type::Comment) {
        continue;
      }

      View::Bytes comment = tokens.get_text(tokens[index]);
      for (Count character = 2; character < comment.get_size(); character++) {
        U8 value = comment[character];
        if (value != ' ' && value != '\t' && value != '\r') {
          return True;
        }
      }
    }

    return False;
  }

  auto classify_shape(Count first, Count last, ScopeRole scope) const -> Shape {
    if (contains_scope(first, last)) {
      return Shape::Block;
    }

    if (scope != ScopeRole::Executable) {
      return Shape::Definition;
    }

    for (Count index = find_unit_start(first, last); index < last; index++) {
      Code::Type code = tokens[index].get_code().get_type();

      if (code == Code::Type::State || code == Code::Type::Const) {
        return Shape::Definition;
      }
    }

    if (find_top_level(first, last, Code::Type::Define) != Count(-1)) {
      return Shape::CompressedBlock;
    }

    return Shape::Statement;
  }

  auto collect_units(
      Count first,
      Count last,
      ScopeRole scope,
      Dynamic::Vector<Unit>& units) const -> void {
    Count current = first;

    while (current < last) {
      Count unit_first = current;
      current = find_unit_start(current, last);

      Count unit_last = find_unit_end(current, last);
      if (unit_last <= unit_first) {
        unit_last = unit_first + 1;
      }

      Section section = classify(unit_first, unit_last, scope);
      Shape shape = classify_shape(unit_first, unit_last, scope);
      Count name = find_name(unit_first, unit_last);
      Bool documented = has_documentation(unit_first, unit_last);
      units.insert({unit_first, unit_last, name, section, shape, documented});
      current = unit_last;
    }
  }

  auto has_attribute(const Unit& unit) const -> Bool {
    Count content = find_unit_start(unit.first, unit.last);
    for (Count index = unit.first; index < content; index++) {
      if (tokens[index].get_code() == Code::Type::Attribute) {
        return True;
      }
    }

    return False;
  }

  auto find_top_level(const Unit& unit, Code::Type selected) const -> Count {
    return find_top_level(unit.first, unit.last, selected);
  }

  auto find_top_level(Count first, Count last, Code::Type selected) const
      -> Count {
    Count packing = 0;
    Count bracket = 0;
    Count scope = 0;

    for (Count index = find_unit_start(first, last); index < last; index++) {
      Code::Type code = tokens[index].get_code().get_type();
      if (!packing && !bracket && !scope && code == selected) {
        return index;
      }

      if (code == Code::Type::PackingStart) {
        packing++;
      } else if (code == Code::Type::PackingEnd && packing) {
        packing--;
      } else if (code == Code::Type::BracketStart) {
        bracket++;
      } else if (code == Code::Type::BracketEnd && bracket) {
        bracket--;
      } else if (code == Code::Type::ScopeStart) {
        scope++;
      } else if (code == Code::Type::ScopeEnd && scope) {
        scope--;
      }
    }

    return Count(-1);
  }

  auto count_top_level(const Unit& unit, Code::Type selected) const -> Count {
    Count packing = 0;
    Count bracket = 0;
    Count scope = 0;
    Count count = 0;

    for (Count index = find_unit_start(unit.first, unit.last);
         index < unit.last; index++) {
      Code::Type code = tokens[index].get_code().get_type();
      if (!packing && !bracket && !scope && code == selected) {
        count++;
      }

      if (code == Code::Type::PackingStart) {
        packing++;
      } else if (code == Code::Type::PackingEnd && packing) {
        packing--;
      } else if (code == Code::Type::BracketStart) {
        bracket++;
      } else if (code == Code::Type::BracketEnd && bracket) {
        bracket--;
      } else if (code == Code::Type::ScopeStart) {
        scope++;
      } else if (code == Code::Type::ScopeEnd && scope) {
        scope--;
      }
    }

    return count;
  }

  auto canonical_token_width(Count index) const -> Count {
    Code::Type code = tokens[index].get_code().get_type();
    View::Bytes text = tokens.get_text(tokens[index]);
    if (code == Code::Type::Attribute) {
      return text.get_size() + 1;
    }

    if (code == Code::Type::Hex && text.get_size() > 2) {
      View::Bytes payload = text.slice(2);
      Count first = 0;
      while (first + 1 < payload.get_size() && payload[first] == '0') {
        first++;
      }

      Count width = 2;
      while (width < payload.get_size() - first) {
        width *= 2;
      }
      return width + 2;
    }

    if (code == Code::Type::Bytes && text.get_size() >= 4 &&
        text[text.get_size() - 1] == ']') {
      Count digits = 0;
      for (Count character = 3; character + 1 < text.get_size(); character++) {
        if (!Lexicon::is_hex(text[character]) &&
            !Lexicon::is_whitespace(text[character])) {
          return text.get_size();
        }
        digits += Lexicon::is_hex(text[character]) ? 1 : 0;
      }

      if (!(digits % 2)) {
        Count spaces = digits > 1 ? (digits / 2) - 1 : 0;
        return 4 + digits + spaces;
      }
    }

    return text.get_size();
  }

  auto measured_prefix(
      Count index,
      Code::Type code,
      Bool has_previous_value,
      Code::Type previous_code,
      Bool previous_was_prefix) const -> Bool {
    if (!has_previous_value) {
      return False;
    }

    if (code == Code::Type::PackingStart) {
      return previous_code == Code::Type::If ||
             previous_code == Code::Type::While ||
             previous_code == Code::Type::Match ||
             previous_code == Code::Type::Return;
    }

    if (code == Code::Type::BracketStart) {
      return previous_code == Code::Type::For ||
             (index && tokens.get_text(tokens[index - 1]) == "stage"_view);
    }

    if (code == Code::Type::AddressOp || code == Code::Type::TypeAccessOp ||
        code == Code::Type::SwizzleOp || code == Code::Type::ValueAccessOp ||
        code == Code::Type::RangeOp || code == Code::Type::PackingEnd ||
        code == Code::Type::BracketEnd || code == Code::Type::NotOp ||
        code == Code::Type::QuestionOp) {
      return False;
    }

    return previous_code != Code::Type::Attribute &&
           previous_code != Code::Type::AddressOp &&
           previous_code != Code::Type::TypeAccessOp &&
           previous_code != Code::Type::SwizzleOp &&
           previous_code != Code::Type::ValueAccessOp &&
           previous_code != Code::Type::RangeOp && !previous_was_prefix &&
           previous_code != Code::Type::Define &&
           previous_code != Code::Type::PackingOp &&
           !is_binary(previous_code) &&
           previous_code != Code::Type::PackingStart &&
           previous_code != Code::Type::BracketStart;
  }

  auto measured_is_prefix(
      Code::Type code,
      Bool has_previous_value,
      Code::Type previous_code) const -> Bool {
    if (code != Code::Type::NotOp && code != Code::Type::SubOp &&
        code != Code::Type::AddOp) {
      return False;
    }

    return !has_previous_value || previous_code == Code::Type::PackingStart ||
           previous_code == Code::Type::BracketStart ||
           previous_code == Code::Type::Assign ||
           previous_code == Code::Type::AddAssign ||
           previous_code == Code::Type::SubAssign || is_binary(previous_code) ||
           previous_code == Code::Type::Return ||
           previous_code == Code::Type::If ||
           previous_code == Code::Type::While ||
           previous_code == Code::Type::Case ||
           previous_code == Code::Type::In ||
           previous_code == Code::Type::PackingOp;
  }

  auto measure(Count first, Count last) const -> Count {
    Count width = 0;
    Code::Type previous_code = Code::Type::Terminal;
    Bool has_previous_value = False;
    Bool previous_was_prefix = False;

    for (Count index = first; index < last; index++) {
      Code::Type code = tokens[index].get_code().get_type();
      Bool prefix = measured_is_prefix(code, has_previous_value, previous_code);

      if (code == Code::Type::Define) {
        width += previous_code == Code::Type::Case ? 0 : 1;
        width++;
        if (index + 1 >= tokens.get_size() ||
            tokens[index + 1].get_code() != Code::Type::Assign) {
          width++;
        }
      } else if (
          code == Code::Type::Assign && previous_code == Code::Type::Define) {
        width += 2;
      } else {
        width += measured_prefix(
                     index, code, has_previous_value, previous_code,
                     previous_was_prefix)
                     ? 1
                     : 0;
        width += canonical_token_width(index);
      }

      if (code == Code::Type::PackingOp || (!prefix && is_binary(code))) {
        width++;
      }

      previous_code = code;
      previous_was_prefix = prefix;
      has_previous_value = True;
    }

    return width;
  }

  auto alignment_kind(const Unit& unit, ScopeRole scope) const
      -> AlignmentKind {
    if (unit.documented || has_comment(unit) || has_attribute(unit) ||
        unit.shape == Shape::Block) {
      return AlignmentKind::None;
    }

    if (scope == ScopeRole::Executable) {
      Count content = find_unit_start(unit.first, unit.last);
      if (content < unit.last &&
          (tokens[content].get_code() == Code::Type::State ||
           tokens[content].get_code() == Code::Type::Const) &&
          find_top_level(unit, Code::Type::Define) != Count(-1)) {
        return AlignmentKind::Local;
      }

      return AlignmentKind::None;
    }

    switch (unit.section) {
    case Section::PublicAlias:
    case Section::PrivateAlias:
      return AlignmentKind::Alias;
    case Section::Constant:
      return AlignmentKind::Constant;
    case Section::StaticVariable:
      return AlignmentKind::StaticVariable;
    case Section::InstanceVariable:
      return AlignmentKind::InstanceVariable;
    default:
      return AlignmentKind::None;
    }
  }

  auto get_alignment(
      const Dynamic::Vector<Unit>& units,
      Count selected,
      ScopeRole scope) const -> Alignment {
    AlignmentKind kind = alignment_kind(units[selected], scope);
    if (kind == AlignmentKind::None) {
      return {};
    }

    Count first = selected;
    while (first && alignment_kind(units[first - 1], scope) == kind) {
      first--;
    }

    Count last = selected + 1;
    while (last < units.get_size() &&
           alignment_kind(units[last], scope) == kind) {
      last++;
    }

    if (last - first < 2) {
      return {};
    }

    Alignment result;
    Count define_max = 0;
    for (Count index = first; index < last; index++) {
      Count define = find_top_level(units[index], Code::Type::Define);
      if (define == Count(-1)) {
        return {};
      }
      define_max = Math::max(
          define_max,
          measure(
              find_unit_start(units[index].first, units[index].last), define));
    }

    Bool eligible = define_max + (indent * 2) + 1 <= 60;
    for (Count index = first; eligible && index < last; index++) {
      Count content = find_unit_start(units[index].first, units[index].last);
      Count define = find_top_level(units[index], Code::Type::Define);
      Count prefix = measure(content, define);
      Count padding = define_max - prefix;
      Count full = measure(content, units[index].last);
      Count unaligned = full + (indent * 2);
      eligible = padding <= 8 &&
                 (unaligned > line_limit || unaligned + padding <= line_limit);
    }

    if (eligible) {
      result.define = (indent * 2) + define_max + 1;
    }

    Count assign_max = 0;
    for (Count index = first; index < last; index++) {
      Count assign = find_top_level(units[index], Code::Type::Assign);
      if (assign == Count(-1) ||
          (assign && tokens[assign - 1].get_code() == Code::Type::Define)) {
        return result;
      }

      Count content = find_unit_start(units[index].first, units[index].last);
      Count prefix = measure(content, assign);
      if (result.define != Count(-1)) {
        Count define = find_top_level(units[index], Code::Type::Define);
        prefix += define_max - measure(content, define);
      }
      assign_max = Math::max(assign_max, prefix);
    }

    Bool assign_eligible = assign_max + (indent * 2) + 1 <= 60;
    for (Count index = first; assign_eligible && index < last; index++) {
      Count content = find_unit_start(units[index].first, units[index].last);
      Count assign = find_top_level(units[index], Code::Type::Assign);
      Count prefix = measure(content, assign);
      Count define_padding = 0;
      if (result.define != Count(-1)) {
        Count define = find_top_level(units[index], Code::Type::Define);
        define_padding = define_max - measure(content, define);
        prefix += define_padding;
      }
      Count padding = assign_max - prefix;
      Count full = measure(content, units[index].last);
      Count unaligned = full + define_padding + (indent * 2);
      assign_eligible = padding <= 8 && (unaligned > line_limit ||
                                         unaligned + padding <= line_limit);
    }

    if (assign_eligible) {
      result.assign = (indent * 2) + assign_max + 1;
    }
    return result;
  }

  auto sorts_by_name(Section section) const -> Bool {
    switch (section) {
    case Section::PublicAlias:
    case Section::PrivateAlias:
    case Section::Constant:
    case Section::PublicStaticFunction:
    case Section::PublicSelfFunction:
    case Section::PrivateStaticFunction:
    case Section::PrivateSelfFunction:
    case Section::PublicType:
    case Section::PrivateType:
      return True;
    default:
      return False;
    }
  }

  auto name_precedes(const Unit& left, const Unit& right) const -> Bool {
    if (left.name >= left.last) {
      return False;
    }

    if (right.name >= right.last) {
      return True;
    }

    View::Bytes left_name = tokens.get_text(tokens[left.name]);
    View::Bytes right_name = tokens.get_text(tokens[right.name]);
    Count index = 0;

    while (index < left_name.get_size() && index < right_name.get_size()) {
      if (left_name[index] != right_name[index]) {
        return left_name[index] < right_name[index];
      }

      index++;
    }

    return left_name.get_size() < right_name.get_size();
  }

  auto shader_stage_rank(const Unit& unit) const -> Option<U8> {
    Count start = find_unit_start(unit.first, unit.last);
    if (start + 1 >= unit.last ||
        tokens[start].get_code() != Code::Type::Type ||
        tokens.get_text(tokens[start]) != "Shader"_view ||
        tokens[start + 1].get_code() != Code::Type::Addressable) {
      return {};
    }

    View::Bytes name = tokens.get_text(tokens[start + 1]);
    if (name == "vertex"_view) {
      return U8(0);
    }
    if (name == "fragment"_view) {
      return U8(1);
    }

    return U8(2);
  }

  auto precedes(const Unit& left, const Unit& right) const -> Bool {
    if (left.section != right.section) {
      return left.section < right.section;
    }

    auto left_stage = shader_stage_rank(left);
    auto right_stage = shader_stage_rank(right);
    if (left_stage && right_stage && *left_stage != *right_stage) {
      return *left_stage < *right_stage;
    }

    return sorts_by_name(left.section) && name_precedes(left, right);
  }

  auto sort_units(Dynamic::Vector<Unit>& units) const -> void {
    for (Count index = 1; index < units.get_size(); index++) {
      Count position = index;

      while (position && precedes(units[position], units[position - 1])) {
        Data::swap(units[position], units[position - 1]);
        position--;
      }
    }
  }

  auto write_units(Count first, Count last, ScopeRole scope) -> void {
    Dynamic::Vector<Unit> units;
    collect_units(first, last, scope, units);

    if (scope != ScopeRole::Executable) {
      sort_units(units);
    }

    for (Count index = 0; index < units.get_size(); index++) {
      units[index].shape = get_output_shape(units[index], scope);
    }

    for (Count index = 0; index < units.get_size(); index++) {
      if (index && separates(units[index - 1], units[index], scope)) {
        write_blank_line();
      }

      write_unit(units[index], scope, True, get_alignment(units, index, scope));
    }
  }

  auto separates(const Unit& previous, const Unit& current, ScopeRole scope)
      const -> Bool {
    Bool alias_block = (previous.section == Section::PublicAlias ||
                        previous.section == Section::PrivateAlias) &&
                       (current.section == Section::PublicAlias ||
                        current.section == Section::PrivateAlias);
    if (scope != ScopeRole::Executable && previous.section != current.section &&
        !alias_block) {
      return True;
    }

    if (current.documented) {
      return True;
    }

    // One paragraph accepts Definitions, Statements, compressed Blocks, then
    // one braced Block. Returning to an earlier stage starts another paragraph.
    return previous.shape == Shape::Block ||
           U8(current.shape) < U8(previous.shape);
  }

  auto get_output_shape(const Unit& unit, ScopeRole scope) const -> Shape {
    if (unit.shape != Shape::Block) {
      return is_function_section(unit.section) ? Shape::CompressedBlock
                                               : unit.shape;
    }

    Count index = find_unit_start(unit.first, unit.last);
    while (index < unit.last) {
      if (tokens[index].get_code() != Code::Type::ScopeStart) {
        index++;
        continue;
      }

      Count closing = find_pair(index, unit.last);
      auto function_shape = get_function_output_shape(unit, index, closing);
      if (function_shape) {
        return *function_shape;
      }

      if (closing >= unit.last ||
          !select_compact_statement(index, closing, unit, scope)) {
        return Shape::Block;
      }

      index = closing + 1;
    }

    return Shape::CompressedBlock;
  }

  auto get_function_output_shape(const Unit& unit, Count opening, Count closing)
      const -> Option<Shape> {
    if (!is_empty_result_function(unit, opening)) {
      return {};
    }

    Dynamic::Vector<Unit> statements;
    collect_units(opening + 1, closing, ScopeRole::Executable, statements);
    if (!statements.get_size()) {
      return Shape::CompressedBlock;
    }

    Count retained = statements.get_size();
    while (retained && is_bare_return(statements[retained - 1])) {
      retained--;
    }

    if (retained == statements.get_size()) {
      return {};
    }

    if (!retained) {
      return Shape::CompressedBlock;
    }

    return retained == 1 && statements[0].shape != Shape::Block
               ? Shape::CompressedBlock
               : Shape::Block;
  }

  auto contains_scope(Count first, Count last) const -> Bool {
    for (Count index = first; index < last; index++) {
      if (tokens[index].get_code() == Code::Type::ScopeStart) {
        return True;
      }
    }

    return False;
  }

  auto write_unit(
      const Unit& unit,
      ScopeRole parent_scope,
      Bool end_line = True,
      Alignment selected_alignment = {}) -> void {
    Alignment saved_alignment = alignment;
    alignment = selected_alignment;
    Count content = find_unit_start(unit.first, unit.last);
    write_documentation(unit.first, content);
    write_attributes(unit.first, content);

    Count index = content;

    while (index < unit.last) {
      Code::Type code = tokens[index].get_code().get_type();

      if (code == Code::Type::ScopeStart) {
        Count closing = find_pair(index, unit.last);
        write_scope(index, closing, unit, parent_scope);
        index = closing < unit.last ? closing + 1 : unit.last;
        continue;
      }

      if (code == Code::Type::PackingStart ||
          code == Code::Type::BracketStart) {
        Count closing = find_pair(index, unit.last);
        if (closing < unit.last) {
          write_group(index, closing);
          index = closing + 1;
          continue;
        }
      }

      write_token(index);
      index++;
    }

    if (end_line) {
      write_newline();
    }
    alignment = saved_alignment;
  }

  auto write_documentation(Count first, Count last) -> void {
    for (Count index = first; index < last; index++) {
      if (tokens[index].get_code().is_comment()) {
        write_comment(tokens[index]);
      }
    }
  }

  auto write_attributes(Count first, Count last) -> void {
    Count current = first;
    Bool wrote_attribute = False;

    while (current < last) {
      if (tokens[current].get_code() != Code::Type::Attribute) {
        current++;
        continue;
      }

      if (wrote_attribute) {
        write_space();
      }

      current = write_attribute(current, last);
      wrote_attribute = True;
    }

    if (wrote_attribute) {
      write_newline();
    }
  }

  auto write_attribute(Count first, Count last) -> Count {
    Count end = first + 1;
    if (end < last && tokens[end].get_code() == Code::Type::PackingStart) {
      Count closing = find_pair(end, last);
      end = closing < last ? closing + 1 : last;
    }

    while (first < end) {
      write_token(first);
      first++;
    }

    return end;
  }

  auto write_scope(
      Count opening,
      Count closing,
      const Unit& unit,
      ScopeRole parent_scope) -> void {
    if (closing > opening + 1 &&
        tokens[closing - 1].get_code() == Code::Type::PackingOp) {
      write_comma_scope(opening, closing);
      return;
    }

    if (write_function_scope(opening, closing, unit)) {
      return;
    }

    auto compact =
        select_compact_statement(opening, closing, unit, parent_scope);
    if (compact) {
      write_compact_statement(*compact);
      return;
    }

    write_braced_scope(opening, closing, unit, parent_scope, closing);
  }

  auto write_comma_scope(Count opening, Count closing) -> void {
    write_token(opening);
    write_newline();
    indent++;

    Count index = opening + 1;
    while (index < closing) {
      Code::Type code = tokens[index].get_code().get_type();
      if (code == Code::Type::PackingStart ||
          code == Code::Type::BracketStart) {
        Count nested_closing = find_pair(index, closing);
        if (nested_closing < closing) {
          write_group(index, nested_closing);
          index = nested_closing + 1;
          continue;
        }
      }

      if (code == Code::Type::PackingOp) {
        append(tokens.get_text(tokens[index]));
        previous = code;
        previous_prefix = False;
        has_previous = True;
        write_newline();
      } else {
        write_token(index);
      }
      index++;
    }

    if (line_started) {
      write_newline();
    }
    indent--;
    continuation_indent = 0;
    write_token(closing);
  }

  auto write_braced_scope(
      Count opening,
      Count closing,
      const Unit& unit,
      ScopeRole parent_scope,
      Count content_end) -> void {
    write_token(opening);

    if (closing >= unit.last) {
      return;
    }

    if (content_end == opening + 1) {
      write_token(closing);
      return;
    }

    write_newline();
    indent++;

    ScopeRole scope = select_scope(unit, opening, parent_scope);
    write_units(opening + 1, content_end, scope);
    indent--;

    ensure_indent();
    append("}"_view);
    previous = Code::Type::ScopeEnd;
    previous_prefix = False;
    has_previous = True;
  }

  auto is_empty_result_function(const Unit& unit, Count opening) const -> Bool {
    if (!is_function_section(unit.section)) {
      return False;
    }

    Count arrow = opening;
    for (Count index = find_unit_start(unit.first, unit.last); index < opening;
         index++) {
      if (tokens[index].get_code() == Code::Type::CallOp) {
        arrow = index;
      }
    }

    if (arrow == opening) {
      return False;
    }

    for (Count index = arrow + 1; index < opening; index++) {
      if (tokens[index].get_code() != Code::Type::BracketStart) {
        continue;
      }

      Count end = find_pair(index, opening);
      return end == index + 1;
    }

    return False;
  }

  auto is_bare_return(const Unit& unit) const -> Bool {
    if (unit.documented || has_comment(unit)) {
      return False;
    }

    Count content = find_unit_start(unit.first, unit.last);
    return content + 2 == unit.last &&
           tokens[content].get_code() == Code::Type::Return &&
           tokens[content + 1].get_code() == Code::Type::EndStatement;
  }

  auto write_empty_function() -> void {
    write_space();
    append(":"_view);
    write_space();
    append("return;"_view);
    previous = Code::Type::EndStatement;
    previous_prefix = False;
    has_previous = True;
  }

  auto write_function_scope(Count opening, Count closing, const Unit& unit)
      -> Bool {
    if (!is_empty_result_function(unit, opening)) {
      return False;
    }

    Dynamic::Vector<Unit> statements;
    collect_units(opening + 1, closing, ScopeRole::Executable, statements);
    if (!statements.get_size()) {
      write_empty_function();
      return True;
    }

    Count retained = statements.get_size();
    while (retained && is_bare_return(statements[retained - 1])) {
      retained--;
    }

    if (retained == statements.get_size()) {
      return False;
    }

    if (!retained) {
      write_empty_function();
      return True;
    }

    if (retained == 1 && statements[0].shape != Shape::Block) {
      write_compact_statement(statements[0]);
      return True;
    }

    write_braced_scope(
        opening, closing, unit, ScopeRole::Executable,
        statements[retained].first);
    return True;
  }

  auto is_function_section(Section section) const -> Bool {
    return section == Section::PublicStaticFunction ||
           section == Section::PublicSelfFunction ||
           section == Section::PrivateStaticFunction ||
           section == Section::PrivateSelfFunction;
  }

  auto is_flow_block(const Unit& unit, Count opening, ScopeRole parent_scope)
      const -> Bool {
    if (is_function_section(unit.section)) {
      return True;
    }

    if (parent_scope != ScopeRole::Executable) {
      return False;
    }

    Count content = find_unit_start(unit.first, unit.last);
    if (content == opening) {
      return True;
    }

    for (Count index = content; index < opening; index++) {
      Code::Type code = tokens[index].get_code().get_type();
      if (code == Code::Type::Match) {
        return False;
      }

      if (code == Code::Type::Func || code == Code::Type::If ||
          code == Code::Type::Else || code == Code::Type::For ||
          code == Code::Type::While || code == Code::Type::Case) {
        return True;
      }
    }

    return False;
  }

  auto select_compact_statement(
      Count opening,
      Count closing,
      const Unit& unit,
      ScopeRole parent_scope) const -> Option<Unit> {
    if (!is_flow_block(unit, opening, parent_scope)) {
      return {};
    }

    Dynamic::Vector<Unit> statements;
    collect_units(opening + 1, closing, ScopeRole::Executable, statements);
    if (statements.get_size() != 1 ||
        statements.get_data()[0].shape == Shape::Block) {
      return {};
    }

    return statements.get_data()[0];
  }

  auto has_comment(const Unit& unit) const -> Bool {
    Count content = find_unit_start(unit.first, unit.last);
    for (Count index = unit.first; index < content; index++) {
      if (tokens[index].get_code().is_comment()) {
        return True;
      }
    }

    return False;
  }

  auto write_compact_statement(const Unit& statement) -> void {
    write_space();
    append(':');
    previous = Code::Type::Define;
    previous_prefix = False;
    has_previous = True;

    if (has_comment(statement)) {
      write_newline();
      indent++;
      write_unit(statement, ScopeRole::Executable);
      indent--;
      return;
    }

    write_space();
    Bool previous_inline = inline_statement;
    inline_statement = True;
    write_unit(statement, ScopeRole::Executable, False);
    inline_statement = previous_inline;
  }

  auto write_group(Count opening, Count closing) -> void {
    Alignment saved_alignment = alignment;
    alignment = {};
    Bool expanded = should_expand(opening, closing);
    Bool trailing = closing > opening + 1 &&
                    tokens[closing - 1].get_code() == Code::Type::PackingOp;
    write_token(opening);

    if (expanded) {
      write_newline();
      indent++;
    }

    Count index = opening + 1;
    while (index < closing) {
      Code::Type code = tokens[index].get_code().get_type();

      if (!expanded && trailing && index == closing - 1) {
        index++;
        continue;
      }

      if (code == Code::Type::PackingStart ||
          code == Code::Type::BracketStart) {
        Count nested_closing = find_pair(index, closing);
        if (nested_closing < closing) {
          write_group(index, nested_closing);
          index = nested_closing + 1;
          continue;
        }
      }

      if (expanded && code == Code::Type::PackingOp) {
        append(tokens.get_text(tokens[index]));
        previous = code;
        previous_prefix = False;
        has_previous = True;
        write_newline();
      } else {
        write_token(index);
      }

      index++;
    }

    if (expanded) {
      if (!trailing) {
        append(',');
      }

      if (line_started) {
        write_newline();
      }

      indent--;
      continuation_indent = 0;
    }

    write_token(closing);
    alignment = saved_alignment;
  }

  auto should_expand(Count opening, Count closing) const -> Bool {
    if (closing == opening + 1) {
      return False;
    }

    if (tokens[closing - 1].get_code() == Code::Type::PackingOp) {
      return True;
    }

    Count packing = 0;
    Count bracket = 0;
    Count scope = 0;
    Bool has_separator = False;
    Bool has_comment = False;
    for (Count index = opening + 1; index < closing; index++) {
      Code::Type code = tokens[index].get_code().get_type();
      if (!packing && !bracket && !scope && code == Code::Type::PackingOp) {
        has_separator = True;
      }
      if (code == Code::Type::PackingStart) {
        packing++;
      } else if (code == Code::Type::PackingEnd && packing) {
        packing--;
      } else if (code == Code::Type::BracketStart) {
        bracket++;
      } else if (code == Code::Type::BracketEnd && bracket) {
        bracket--;
      } else if (code == Code::Type::ScopeStart) {
        scope++;
      } else if (code == Code::Type::ScopeEnd && scope) {
        scope--;
      }
      has_comment |= tokens[index].get_code().is_comment();
    }

    if (!has_separator) {
      return False;
    }
    if (has_comment) {
      return True;
    }

    Count width = measure(opening, closing + 1);
    Bool measures_suffix =
        closing + 1 < tokens.get_size() &&
        (tokens[closing + 1].get_code() == Code::Type::Define ||
         tokens[closing + 1].get_code() == Code::Type::CallOp);
    Count suffix = measures_suffix ? measure_group_suffix(closing) : 0;
    return column + width + suffix > line_limit;
  }

  auto measure_group_suffix(Count closing) const -> Count {
    Count end = closing + 1;
    while (end < tokens.get_size()) {
      Code::Type code = tokens[end].get_code().get_type();
      if (code == Code::Type::PackingStart ||
          code == Code::Type::BracketStart || code == Code::Type::ScopeStart ||
          code == Code::Type::PackingEnd || code == Code::Type::BracketEnd ||
          code == Code::Type::ScopeEnd) {
        break;
      }

      end++;
      if (code == Code::Type::PackingOp || code == Code::Type::EndStatement) {
        break;
      }
    }

    if (end == closing + 1) {
      return 0;
    }

    return measure(closing, end) - canonical_token_width(closing);
  }

  auto select_scope(const Unit& unit, Count opening, ScopeRole parent_scope)
      const -> ScopeRole {
    if (parent_scope == ScopeRole::Executable) {
      return ScopeRole::Executable;
    }

    if (is_function_section(unit.section)) {
      return ScopeRole::Executable;
    }

    for (Count index = unit.first; index < opening; index++) {
      Code::Type code = tokens[index].get_code().get_type();

      if (code == Code::Type::Func || code == Code::Type::If ||
          code == Code::Type::Else || code == Code::Type::For ||
          code == Code::Type::While || code == Code::Type::Match ||
          code == Code::Type::Case) {
        return ScopeRole::Executable;
      }

      if (code == Code::Type::Struct || code == Code::Type::Object ||
          code == Code::Type::Interface || code == Code::Type::Implementation) {
        return ScopeRole::Instance;
      }

      if (code == Code::Type::Namespace) {
        return ScopeRole::Static;
      }

      if (code == Code::Type::Enum) {
        return ScopeRole::Enumeration;
      }
    }

    return ScopeRole::Static;
  }

  auto write_token(Count index) -> void {
    Code::Type code = tokens[index].get_code().get_type();
    View::Bytes text = tokens.get_text(tokens[index]);
    Bool prefix = is_prefix(code);

    if ((code == Code::Type::PackingEnd || code == Code::Type::BracketEnd) &&
        !line_started) {
      continuation_indent = 0;
    }

    Bool break_postfix =
        code == Code::Type::SwizzleOp || code == Code::Type::ValueAccessOp;
    if (line_started &&
        ((has_previous && is_binary(previous)) || break_postfix) &&
        column + text.get_size() > line_limit) {
      write_newline(2);
    }

    if (tokens[index].get_code().is_comment()) {
      write_comment(tokens[index]);
      return;
    }

    if (code == Code::Type::Attribute) {
      append("@"_view);
      append(text);
      previous = code;
      previous_prefix = False;
      has_previous = True;
      return;
    }

    if (code == Code::Type::ScopeStart) {
      write_space();
      append(text);
      previous = code;
      previous_prefix = False;
      has_previous = True;
      return;
    }

    if (code == Code::Type::Hex || code == Code::Type::Bytes) {
      if (requires_space(index, code)) {
        write_space();
      }

      if (code == Code::Type::Hex) {
        write_hex(text);
      } else if (!write_bytes(text)) {
        append(text);
      }

      previous = code;
      previous_prefix = False;
      has_previous = True;
      return;
    }

    if (code == Code::Type::ScopeEnd || code == Code::Type::PackingEnd ||
        code == Code::Type::BracketEnd || code == Code::Type::EndStatement ||
        code == Code::Type::PackingOp || code == Code::Type::QuestionOp) {
      append(text);
    } else if (code == Code::Type::Define) {
      if (previous != Code::Type::Case) {
        if (alignment.define != Count(-1)) {
          write_padding(alignment.define);
        } else {
          write_space();
        }
      }

      append(text);
      if (index + 1 >= tokens.get_size() ||
          tokens[index + 1].get_code() != Code::Type::Assign) {
        write_space();
      }
    } else if (code == Code::Type::Assign && previous == Code::Type::Define) {
      append(text);
      write_space();
    } else if (code == Code::Type::Assign && alignment.assign != Count(-1)) {
      write_padding(alignment.assign);
      append(text);
    } else if (prefix) {
      if (prefix_requires_space()) {
        write_space();
      }

      append(text);
    } else if (requires_space(index, code)) {
      write_space();
      append(text);
    } else {
      append(text);
    }

    if (code == Code::Type::EndStatement) {
      Bool continues_alternate =
          index + 1 < tokens.get_size() &&
          tokens[index + 1].get_code() == Code::Type::Else;
      if (inline_statement || continues_alternate) {
        previous = code;
        previous_prefix = False;
        has_previous = True;
      } else {
        write_newline();
      }
      return;
    } else if (code == Code::Type::PackingOp) {
      if (column >= line_limit - 10) {
        write_newline(1);
      } else {
        write_space();
      }
    } else if (!prefix && is_binary(code)) {
      if (column >= line_limit - 20) {
        write_newline(2);
      } else {
        write_space();
      }
    }

    previous = code;
    previous_prefix = prefix;
    has_previous = True;
  }

  auto is_prefix(Code::Type code) const -> Bool {
    if (code != Code::Type::NotOp && code != Code::Type::SubOp &&
        code != Code::Type::AddOp) {
      return False;
    }

    return !has_previous || previous == Code::Type::PackingStart ||
           previous == Code::Type::BracketStart ||
           previous == Code::Type::Assign ||
           previous == Code::Type::AddAssign ||
           previous == Code::Type::SubAssign || is_binary(previous) ||
           previous == Code::Type::Return || previous == Code::Type::If ||
           previous == Code::Type::While || previous == Code::Type::Case ||
           previous == Code::Type::In || previous == Code::Type::PackingOp;
  }

  auto prefix_requires_space() const -> Bool {
    if (!has_previous || !line_started) {
      return False;
    }

    return previous != Code::Type::PackingStart &&
           previous != Code::Type::BracketStart &&
           previous != Code::Type::PackingOp && !is_binary(previous);
  }

  auto requires_space(Count index, Code::Type code) const -> Bool {
    if (!has_previous || !line_started) {
      return False;
    }

    if (code == Code::Type::PackingStart) {
      return previous == Code::Type::If || previous == Code::Type::While ||
             previous == Code::Type::Match || previous == Code::Type::Return;
    }

    if (code == Code::Type::BracketStart) {
      return previous == Code::Type::For ||
             (index && tokens.get_text(tokens[index - 1]) == "stage"_view);
    }

    if (code == Code::Type::AddressOp || code == Code::Type::TypeAccessOp ||
        code == Code::Type::SwizzleOp || code == Code::Type::ValueAccessOp ||
        code == Code::Type::RangeOp || code == Code::Type::PackingEnd ||
        code == Code::Type::BracketEnd || code == Code::Type::NotOp ||
        code == Code::Type::QuestionOp) {
      return False;
    }

    if (previous == Code::Type::Attribute ||
        previous == Code::Type::AddressOp ||
        previous == Code::Type::TypeAccessOp ||
        previous == Code::Type::SwizzleOp ||
        previous == Code::Type::ValueAccessOp ||
        previous == Code::Type::RangeOp || previous_prefix ||
        previous == Code::Type::PackingStart ||
        previous == Code::Type::BracketStart) {
      return False;
    }

    return True;
  }

  auto is_binary(Code::Type code) const -> Bool {
    return code == Code::Type::AddOp || code == Code::Type::SubOp ||
           code == Code::Type::DivOp || code == Code::Type::MulOp ||
           code == Code::Type::ModOp || code == Code::Type::LessOp ||
           code == Code::Type::GreaterOp || code == Code::Type::LessEqOp ||
           code == Code::Type::GreaterEqOp || code == Code::Type::CmpOp ||
           code == Code::Type::NotEqOp || code == Code::Type::Assign ||
           code == Code::Type::AddAssign || code == Code::Type::SubAssign ||
           code == Code::Type::CallOp || code == Code::Type::And ||
           code == Code::Type::Or || code == Code::Type::In;
  }

  auto write_comment(Token token) -> void {
    if (line_started) {
      write_space();
    }

    View::Bytes text = tokens.get_text(token);
    if (token.get_code() == Code::Type::RawComment) {
      append(text);
      write_newline();
      return;
    }

    View::Bytes marker = Lexicon::get_spelling(Code::Type::Comment);
    View::Bytes content = text.slice(marker.get_size());
    if (!content.is_empty() && content[0] == ' ') {
      content = content.slice(1);
    }

    append(marker);
    if (!content.is_empty()) {
      write_space();
      append(content);
    }

    write_newline();
  }

  constexpr auto uppercase_hex(U8 value) const -> U8 {
    return value >= 'a' && value <= 'f' ? value - ('a' - 'A') : value;
  }

  auto write_hex(View::Bytes text) -> void {
    constexpr Count prefix_size = 2;
    if (text.get_size() <= prefix_size) {
      append(text);
      return;
    }

    View::Bytes payload = text.slice(prefix_size);
    Count first = 0;
    while (first + 1 < payload.get_size() && payload[first] == '0') {
      first++;
    }

    Count significant = payload.get_size() - first;
    Count width = 2;
    while (width < significant) {
      width *= 2;
    }

    append("0x"_view);
    for (Count index = significant; index < width; index++) {
      append('0');
    }

    for (Count index = first; index < payload.get_size(); index++) {
      append(uppercase_hex(payload[index]));
    }
  }

  auto write_bytes(View::Bytes text) -> Bool {
    constexpr Count prefix_size = 3;
    if (text.get_size() < prefix_size + 1 || text[0] != '0' || text[1] != 'x' ||
        text[2] != '[' || text[text.get_size() - 1] != ']') {
      return False;
    }

    Count digits = 0;
    for (Count index = prefix_size; index + 1 < text.get_size(); index++) {
      U8 value = text[index];
      if (!Lexicon::is_hex(value) && !Lexicon::is_whitespace(value)) {
        return False;
      }

      digits += Lexicon::is_hex(value) ? 1 : 0;
    }

    if (digits % 2) {
      return False;
    }

    append("0x["_view);
    Count emitted = 0;
    for (Count index = prefix_size; index + 1 < text.get_size(); index++) {
      if (!Lexicon::is_hex(text[index])) {
        continue;
      }

      if (emitted && emitted % 2 == 0) {
        write_space();
      }

      append(uppercase_hex(text[index]));
      emitted++;
    }

    append(']');
    return True;
  }

  auto ensure_indent() -> void {
    if (line_started) {
      return;
    }

    output.append(' ', (indent + continuation_indent) * 2);
    column = (indent + continuation_indent) * 2;
    continuation_indent = 0;
    line_started = True;
  }

  auto write_space() -> void {
    ensure_indent();

    if (output.get_size() && output[output.get_size() - 1] != ' ') {
      output.append(' ');
      column++;
    }
  }

  auto write_padding(Count target) -> void {
    ensure_indent();
    if (column >= target) {
      write_space();
      return;
    }

    Count padding = target - column;
    output.append(' ', padding);
    column += padding;
  }

  auto write_newline(Count next_continuation = 0) -> void {
    while (output.get_size() && output[output.get_size() - 1] == ' ') {
      output.resize(output.get_size() - 1);
    }

    if (!output.get_size() || output[output.get_size() - 1] != '\n') {
      output.append('\n');
    }

    column = 0;
    continuation_indent = next_continuation;
    line_started = False;
    has_previous = False;
    previous_prefix = False;
  }

  auto write_blank_line() -> void {
    write_newline();

    if (output.get_size() < 2 || output[output.get_size() - 2] != '\n') {
      output.append('\n');
    }
  }

  auto append(View::Bytes text) -> void {
    ensure_indent();
    output.concat(text);
    column += text.get_size();
  }

  auto append(U8 value) -> void {
    ensure_indent();
    output.append(value);
    column++;
  }

  auto finish() -> void {
    while (output.get_size() && (output[output.get_size() - 1] == ' ' ||
                                 output[output.get_size() - 1] == '\n')) {
      output.resize(output.get_size() - 1);
    }

    output.append('\n');
  }

  const Stream& tokens;
  Dynamic::Bytes output;
  Count indent = 0;
  Count continuation_indent = 0;
  Count column = 0;
  Code::Type previous = Code::Type::Terminal;
  Bool line_started = False;
  Bool has_previous = False;
  Bool previous_prefix = False;
  Bool inline_statement = False;
  Alignment alignment;
};

auto Formatter::format() const -> Dynamic::Bytes {
  State state(tokenizer);
  return state.format();
}
