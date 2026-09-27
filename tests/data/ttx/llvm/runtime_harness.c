// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors
//
#include <stddef.h>
#include <stdint.h>

#ifndef TTX_LLVM_HEADER
#define TTX_LLVM_HEADER "Validation.Runtime/1.0/c_abi.h"
#endif

#include TTX_LLVM_HEADER

_Static_assert(
    sizeof(ttx_validation_runtime_Runtime_Pair) == 16,
    "Pair must use its SysV carrier");
_Static_assert(
    offsetof(ttx_validation_runtime_Runtime_Pair, left) == 0,
    "Pair.left offset changed");
_Static_assert(
    offsetof(ttx_validation_runtime_Runtime_Pair, right) == 8,
    "Pair.right offset changed");
_Static_assert(
    sizeof(ttx_validation_runtime_Runtime_Large) == 24,
    "Large must use its SysV carrier");
_Static_assert(
    offsetof(ttx_validation_runtime_Runtime_Large, third) == 16,
    "Large.third offset changed");
_Static_assert(
    sizeof(
        ttx_results_TTX_5fFUNC_5fValidation_5f2eRuntime_5f_5fRuntime_5f_5fresults_5fstatic) ==
        16,
    "multiple results must use their named carrier");
_Static_assert(
    sizeof(ttx_validation_runtime_Runtime_Maybe) == 16,
    "Option must store its payload and selected state inline");
_Static_assert(
    offsetof(ttx_validation_runtime_Runtime_Maybe, value) == 0,
    "Option payload offset changed");
_Static_assert(
    offsetof(ttx_validation_runtime_Runtime_Maybe, set) == 8,
    "Option selected state offset changed");
_Static_assert(
    sizeof(ttx_validation_runtime_Runtime_Outcome) == 16,
    "Result must store one inline alternative and selected state");
_Static_assert(
    offsetof(ttx_validation_runtime_Runtime_Outcome, value_selected) == 8,
    "Result selected state offset changed");
_Static_assert(
    sizeof(ttx_validation_runtime_Runtime_ObjectOutcome) == 16,
    "Object Result must store one handle and selected state inline");
_Static_assert(
    sizeof(ttx_validation_runtime_Runtime_ObjectError) == 16,
    "Object error Result must store one handle and selected state inline");
_Static_assert(
    sizeof(ttx_validation_runtime_Runtime_WideOutcome) == 32,
    "Wide Result must align its largest union alternative");
_Static_assert(
    offsetof(ttx_validation_runtime_Runtime_WideOutcome, value_selected) == 24,
    "Wide Result selected state offset changed");
_Static_assert(
    sizeof(ttx_validation_runtime_Runtime_Access_5bU64_5d) == 16,
    "Access must use its pointer and size carrier");
_Static_assert(
    sizeof(ttx_validation_runtime_Runtime_Counter) == sizeof(void*),
    "Object must use one opaque pointer carrier");
_Static_assert(
    sizeof(ttx_validation_runtime_Runtime_Option_5bCounter_5d) == sizeof(void*),
    "Option[Object] must use the null handle niche");

extern uint64_t dense_storage[];
extern uint64_t printed_value;
extern size_t printed_count;

int run_runtime_integration(void) {
  TTX_FUNC_Validation_2eRuntime__Runtime__runtime_static();
  const uint64_t object_static_value =
      TTX_FUNC_Validation_2eRuntime__Runtime__object_5fstatic_5fvalue_static();
  const uint64_t object_behavior =
      TTX_FUNC_Validation_2eRuntime__Runtime__object_5fbehavior_static();
  const ttx_validation_runtime_Runtime_Pair pair =
      TTX_FUNC_Validation_2eRuntime__Runtime__pair_static();
  const ttx_validation_runtime_Runtime_Large large =
      TTX_FUNC_Validation_2eRuntime__Runtime__large_static();
  const ttx_results_TTX_5fFUNC_5fValidation_5f2eRuntime_5f_5fRuntime_5f_5fresults_5fstatic
      results = TTX_FUNC_Validation_2eRuntime__Runtime__results_static();
  const ttx_validation_runtime_Runtime_Maybe absent =
      llvm_option((ttx_validation_runtime_Runtime_Maybe){
        .value = UINT64_MAX,
        .set = false,
      });
  const ttx_validation_runtime_Runtime_Maybe present =
      llvm_option((ttx_validation_runtime_Runtime_Maybe){
        .value = UINT64_C(37),
        .set = true,
      });
  const ttx_validation_runtime_Runtime_Maybe stopped =
      TTX_FUNC_Validation_2eRuntime__Runtime__bool_5fpropagation_static(false);
  const ttx_validation_runtime_Runtime_Maybe continued =
      TTX_FUNC_Validation_2eRuntime__Runtime__bool_5fpropagation_static(true);
  const ttx_validation_runtime_Runtime_Outcome result_value =
      TTX_FUNC_Validation_2eRuntime__Runtime__result_5fpropagation_static(
          (ttx_validation_runtime_Runtime_Outcome){
            .value = UINT64_C(41),
            .value_selected = true,
          });
  const ttx_validation_runtime_Runtime_Outcome result_error =
      TTX_FUNC_Validation_2eRuntime__Runtime__result_5fpropagation_static(
          (ttx_validation_runtime_Runtime_Outcome){
            .error = true,
            .value_selected = false,
          });
  const ttx_validation_runtime_Runtime_WideOutcome wide_error =
      TTX_FUNC_Validation_2eRuntime__Runtime__result_5fwide_5fpropagation_static(
          (ttx_validation_runtime_Runtime_WideOutcome){
            .error =
                {
                  .first = UINT64_C(4),
                  .second = UINT64_C(5),
                  .third = UINT64_C(6),
                },
            .value_selected = false,
          });
  const uint64_t access_total =
      TTX_FUNC_Validation_2eRuntime__Runtime__access_5fwrite_static(0);
  ttx_validation_runtime_Runtime_Counter object =
      TTX_FUNC_Validation_2eRuntime__Runtime__object_5fcreate_static();
  if (object == NULL) {
    return 1;
  }
  const uint64_t initial_object =
      TTX_FUNC_Validation_2eRuntime__Runtime__object_5fread_static(object);
  const ttx_validation_runtime_Runtime_ObjectOutcome object_value =
      TTX_FUNC_Validation_2eRuntime__Runtime__result_5fobject_static(
          object, false);
  const ttx_validation_runtime_Runtime_ObjectOutcome object_error =
      TTX_FUNC_Validation_2eRuntime__Runtime__result_5fobject_static(
          object, true);
  const ttx_validation_runtime_Runtime_ObjectOutcome propagated_object =
      TTX_FUNC_Validation_2eRuntime__Runtime__result_5fobject_5fpropagation_static(
          (ttx_validation_runtime_Runtime_ObjectOutcome){
            .value = object,
            .value_selected = true,
          });
  const ttx_validation_runtime_Runtime_ObjectError propagated_object_error =
      TTX_FUNC_Validation_2eRuntime__Runtime__result_5ferror_5fobject_5fpropagation_static(
          (ttx_validation_runtime_Runtime_ObjectError){
            .error = object,
            .value_selected = false,
          });
  const int object_result_valid =
      object_value.value_selected && object_value.value == object &&
      !object_error.value_selected && object_error.error &&
      propagated_object.value_selected && propagated_object.value == object &&
      !propagated_object_error.value_selected &&
      propagated_object_error.error == object;
  if (object_value.value_selected) {
    perimortem_core_object_release(object_value.value);
  }
  if (propagated_object.value_selected) {
    perimortem_core_object_release(propagated_object.value);
  }
  if (!propagated_object_error.value_selected) {
    perimortem_core_object_release(propagated_object_error.error);
  }
  const ttx_results_TTX_5fFUNC_5fValidation_5f2eRuntime_5f_5fRuntime_5f_5fobject_5f5fresults_5fstatic
      object_results =
          TTX_FUNC_Validation_2eRuntime__Runtime__object_5fresults_static(
              object);
  const int object_results_valid =
      object_results.optional == object && object_results.count == UINT64_C(7);
  if (object_results.optional) {
    perimortem_core_object_release(object_results.optional);
  }
  perimortem_core_object_retain(object);
  perimortem_core_object_release(object);
  const uint64_t changed_object =
      TTX_FUNC_Validation_2eRuntime__Runtime__object_5fadd_static(
          object, UINT64_C(5));
  const uint64_t read_object =
      TTX_FUNC_Validation_2eRuntime__Runtime__object_5fread_static(object);
  perimortem_core_object_release(object);
  if (pair.left != UINT64_C(9) || pair.right != UINT64_C(4) ||
      large.first != UINT64_C(4) || large.second != UINT64_C(5) ||
      large.third != UINT64_C(6) || llvm_large_sum(large) != UINT64_C(15) ||
      TTX_FUNC_Validation_2eRuntime__Runtime__small_static(UINT8_C(9), true) !=
          UINT8_C(9) ||
      results.left != UINT64_C(7) || !results.active || absent.set ||
      !present.set || present.value != UINT64_C(37) || stopped.set ||
      !continued.set || continued.value != UINT64_C(31) ||
      !result_value.value_selected || result_value.value != UINT64_C(41) ||
      result_error.value_selected || !result_error.error ||
      wide_error.value_selected || wide_error.error.first != UINT64_C(4) ||
      wide_error.error.second != UINT64_C(5) ||
      wide_error.error.third != UINT64_C(6) ||
      TTX_FUNC_Validation_2eRuntime__Runtime__result_5ferror_static(
          result_value) ||
      !TTX_FUNC_Validation_2eRuntime__Runtime__result_5ferror_static(
          result_error) ||
      TTX_FUNC_Validation_2eRuntime__Runtime__execute_static() !=
          UINT64_C(21) ||
      access_total != UINT64_C(91) ||
      TTX_FUNC_Validation_2eRuntime__Runtime__bytes_5fconcat_5fsize_static() !=
          UINT64_C(5) ||
      TTX_FUNC_Validation_2eRuntime__Runtime__bytes_5fapi_static() !=
          UINT64_C(777) ||
      TTX_FUNC_Validation_2eRuntime__Runtime__object_5fiteration_static() !=
          UINT64_C(6) ||
      TTX_FUNC_Validation_2eRuntime__Runtime__object_5fstorage_static() !=
          UINT64_C(18)) {
    return 1;
  }
  if (initial_object != UINT64_C(7) || changed_object != UINT64_C(12) ||
      read_object != UINT64_C(12) || !object_results_valid ||
      !object_result_valid) {
    return 2;
  }
  if (object_static_value != UINT64_C(2)) {
    return 3;
  }
  if (object_behavior != UINT64_C(44)) {
    return 4;
  }
  if (TTX_FUNC_Validation_2eRuntime__Runtime__borrow_5fiteration_static() !=
          UINT64_C(197) ||
      printed_count != 1 || printed_value != UINT64_C(21) ||
      dense_storage[0] != UINT64_C(1) || dense_storage[1] != UINT64_C(20) ||
      dense_storage[2] != UINT64_C(37) || dense_storage[3] != UINT64_C(47)) {
    return 5;
  }
  if (TTX_FUNC_Validation_2eRuntime__Runtime__enumeration_5fiteration_static() !=
      UINT64_C(22)) {
    return 6;
  }
  return 0;
}

#ifdef TTX_STANDALONE_INTEGRATION
int main(void) {
  return run_runtime_integration();
}
#endif
