// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/diagnostics/suggestions.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/math.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/managed/bytes.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;

static auto format(Allocator::Arena& arena, View::Bytes candidate)
    -> View::Bytes {
  Managed::Bytes hint(arena);
  hint.concat("Did you mean `"_view);
  hint.concat(candidate);
  hint.concat("`?"_view);
  return hint;
}

static auto distance(View::Bytes left, View::Bytes right) -> Count {
  Count maximum_distance =
      Math::max(left.get_size(), right.get_size()) <= 4 ? Count(1) : Count(2);

  // Use the shorter name for rows so work scales with the smaller input. The
  // final cell remains within the band because names farther apart in length
  // than the accepted distance cannot be suggestions.
  if (left.get_size() > right.get_size()) {
    Data::swap(left, right);
  }

  if (right.get_size() - left.get_size() > maximum_distance) {
    return Count(-1);
  }

  // At most five cells are live in either row for the diagnostic limit of two
  // edits. Eight slots let absolute columns wrap with a binary mask without
  // any live cells colliding, regardless of the input lengths.
  constexpr Count row_size = 8;
  constexpr Count row_mask = row_size - 1;
  U8 rows[2][row_size];
  Count previous_row = 0;
  Count current_row = 1;
  U8 unreachable = U8(maximum_distance + 1);
  for (Count row = 0; row < 2; row++) {
    for (Count slot = 0; slot < row_size; slot++) {
      rows[row][slot] = unreachable;
    }
  }

  Count initial_end = Math::min(right.get_size(), maximum_distance);
  for (Count column = 0; column <= initial_end; column++) {
    rows[previous_row][column & row_mask] = U8(column);
  }

  for (Count i = 1; i <= left.get_size(); i++) {
    // This row previously held older distances. Resetting all eight slots
    // prevents wrapped columns outside the active band from looking reachable.
    for (Count slot = 0; slot < row_size; slot++) {
      rows[current_row][slot] = unreachable;
    }

    Count first_column = i > maximum_distance ? i - maximum_distance : 0;
    Count last_column = Math::min(right.get_size(), i + maximum_distance);
    U8 row_minimum = unreachable;
    if (first_column == 0) {
      rows[current_row][0] = U8(i);
      row_minimum = rows[current_row][0];
    }

    Count column = Math::max(Count(1), first_column);
    for (; column <= last_column; column++) {
      U8 deletion = U8(rows[previous_row][column & row_mask] + 1);
      U8 insertion = U8(rows[current_row][(column - 1) & row_mask] + 1);
      U8 substitution =
          U8(rows[previous_row][(column - 1) & row_mask] +
             (left[i - 1] == right[column - 1] ? 0 : 1));
      U8 candidate_distance = Math::min(
          unreachable, Math::min(Math::min(deletion, insertion), substitution));
      rows[current_row][column & row_mask] = candidate_distance;
      row_minimum = Math::min(row_minimum, candidate_distance);
    }

    // Once every reachable prefix exceeds the limit, later rows cannot return
    // to an accepted edit path.
    if (row_minimum > maximum_distance) {
      return Count(-1);
    }

    Data::swap(previous_row, current_row);
  }

  Count result = rows[previous_row][right.get_size() & row_mask];
  return result <= maximum_distance ? result : Count(-1);
}

static auto consider(
    View::Bytes name,
    View::Bytes candidate,
    Count index,
    Count& best_index) -> Bool {
  if (candidate.is_empty()) {
    return False;
  }

  Count candidate_distance = distance(name, candidate);
  if (candidate_distance <= 1) {
    best_index = index;
    return True;
  }

  if (candidate_distance == 2 && best_index == Count(-1)) {
    best_index = index;
  }

  return False;
}

auto Tetrodotoxin::Diagnostics::Suggestions::possible_candidate(
    Allocator::Arena& arena,
    View::Bytes name,
    View::Vector<View::Bytes> candidates) -> View::Bytes {
  const auto* candidate_data = candidates.get_data();
  Count best_index = Count(-1);
  for (Count i = 0; i < candidates.get_size(); i++) {
    if (consider(name, candidate_data[i], i, best_index)) {
      break;
    }
  }

  return best_index == Count(-1) ? View::Bytes()
                                 : format(arena, candidate_data[best_index]);
}
