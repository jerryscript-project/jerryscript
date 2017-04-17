/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef JERRYX_ARG_H
#define JERRYX_ARG_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "jerryscript.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/**
 * The forward declaration of jerryx_arg_t.
 */
typedef struct jerryx_arg_t jerryx_arg_t;

/**
 * The forward declaration of jerryx_arg_js_iterator_t
 */
typedef struct jerryx_arg_js_iterator_t jerryx_arg_js_iterator_t;

/**
 * Signature of the transform function.
 */
typedef jerry_value_t (*jerryx_arg_transform_func_t) (jerryx_arg_js_iterator_t *js_arg_iter_p, /**< available JS args */
                                                      const jerryx_arg_t *c_arg_p); /**< native arg */

/**
 * The structure defining a single validation & transformation step.
 */
struct jerryx_arg_t
{
  jerryx_arg_transform_func_t func; /**< the transform function */
  void *dest; /**< pointer to destination where func should store the result */
  uintptr_t extra_info; /**< extra information, specific to func */
};

jerry_value_t jerryx_arg_transform_this_and_args (const jerry_value_t this_val,
                                                  const jerry_value_t *js_arg_p,
                                                  const jerry_length_t js_arg_cnt,
                                                  const jerryx_arg_t *c_arg_p,
                                                  jerry_length_t c_arg_cnt);

jerry_value_t jerryx_arg_transform_args (const jerry_value_t *js_arg_p,
                                         const jerry_length_t js_arg_cnt,
                                         const jerryx_arg_t *c_arg_p,
                                         jerry_length_t c_arg_cnt);

/**
 * Indicates whether an argument is allowed to be coerced into the expected JS type.
 */
typedef enum
{
  JERRYX_ARG_COERCE, /**< the transform inside will invoke toNumber, toBoolean or toString */
  JERRYX_ARG_NO_COERCE /**< the type coercion is not allowed. */
} jerryx_arg_coerce_t;

/**
 * Indicates whether an argument is optional or required.
 */
typedef enum
{
  /**
   * The argument is optional. If the argument is `undefined` the transform is
   * successful and `c_arg_p->dest` remains untouched.
   */
  JERRYX_ARG_OPTIONAL,
  /**
   * The argument is required. If the argument is `undefined` the transform
   * will fail and `c_arg_p->dest` remains untouched.
   */
  JERRYX_ARG_REQUIRED
} jerryx_arg_optional_t;

/* Inline functions for initializing jerryx_arg_t */
static inline jerryx_arg_t
jerryx_arg_number (double *dest, jerryx_arg_coerce_t conv_flag, jerryx_arg_optional_t opt_flag);
static inline jerryx_arg_t
jerryx_arg_boolean (bool *dest, jerryx_arg_coerce_t conv_flag, jerryx_arg_optional_t opt_flag);
static inline jerryx_arg_t
jerryx_arg_string (char *dest, uint32_t size, jerryx_arg_coerce_t conv_flag, jerryx_arg_optional_t opt_flag);
static inline jerryx_arg_t
jerryx_arg_function (jerry_value_t *dest, jerryx_arg_optional_t opt_flag);
static inline jerryx_arg_t
jerryx_arg_native_pointer (void **dest, const jerry_object_native_info_t *info_p, jerryx_arg_optional_t opt_flag);
static inline jerryx_arg_t
jerryx_arg_ignore (void);
static inline jerryx_arg_t
jerryx_arg_custom (void *dest, uintptr_t extra_info, jerryx_arg_transform_func_t func);

jerry_value_t
jerryx_arg_transform_optional (jerryx_arg_js_iterator_t *js_arg_iter_p,
                               const jerryx_arg_t *c_arg_p,
                               jerryx_arg_transform_func_t func);

/* Helper functions for transform functions. */
jerry_value_t jerryx_arg_js_iterator_pop (jerryx_arg_js_iterator_t *js_arg_iter_p);
jerry_value_t jerryx_arg_js_iterator_peek (jerryx_arg_js_iterator_t *js_arg_iter_p);
jerry_length_t jerryx_arg_js_iterator_index (jerryx_arg_js_iterator_t *js_arg_iter_p);

#include "arg.impl.h"

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* !JERRYX_ARG_H */
