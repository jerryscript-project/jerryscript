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

#include "ecma-builtin-helpers.h"
#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "ecma-typedarray-object.h"
#include "ecma-try-catch-macro.h"
#include "jrt.h"
#include "jrt-libc-includes.h"
#include "ecma-gc.h"

#ifndef CONFIG_DISABLE_TYPEDARRAY_BUILTIN

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-typedarray-prototype.inc.h"
#define BUILTIN_UNDERSCORED_ID typedarray_prototype
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup typedarrayprototype ECMA %TypedArray%.prototype object built-in
 * @{
 */

/**
 * The %TypedArray%.prototype.buffer accessor
 *
 * See also:
 *          ES2015, 22.2.3.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_buffer_getter (ecma_value_t this_arg) /**< this argument */
{
  if (ecma_is_typedarray (this_arg))
  {
    ecma_object_t *typedarray_p = ecma_get_object_from_value (this_arg);
    ecma_object_t *obj_p = ecma_typedarray_get_arraybuffer (typedarray_p);
    ecma_ref_object (obj_p);

    return ecma_make_object_value (obj_p);
  }

  return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a TypedArray."));
} /* ecma_builtin_typedarray_prototype_buffer_getter */

/**
 * The %TypedArray%.prototype.byteLength accessor
 *
 * See also:
 *          ES2015, 22.2.3.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_bytelength_getter (ecma_value_t this_arg) /**< this argument */
{
  if (ecma_is_typedarray (this_arg))
  {
    ecma_object_t *typedarray_p = ecma_get_object_from_value (this_arg);
    uint8_t shift = ecma_typedarray_get_element_size_shift (typedarray_p);

    return ecma_make_uint32_value (ecma_typedarray_get_length (typedarray_p) << shift);
  }

  return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a TypedArray."));
} /* ecma_builtin_typedarray_prototype_bytelength_getter */

/**
 * The %TypedArray%.prototype.byteOffset accessor
 *
 * See also:
 *          ES2015, 22.2.3.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_byteoffset_getter (ecma_value_t this_arg) /**< this argument */
{
  if (ecma_is_typedarray (this_arg))
  {
    ecma_object_t *typedarray_p = ecma_get_object_from_value (this_arg);

    return ecma_make_uint32_value (ecma_typedarray_get_offset (typedarray_p));
  }

  return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a TypedArray."));
} /* ecma_builtin_typedarray_prototype_byteoffset_getter */

/**
 * The %TypedArray%.prototype.length accessor
 *
 * See also:
 *          ES2015, 22.2.3.17
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
static ecma_value_t
ecma_builtin_typedarray_prototype_length_getter (ecma_value_t this_arg) /**< this argument */
{
  if (ecma_is_typedarray (this_arg))
  {
    ecma_object_t *typedarray_p = ecma_get_object_from_value (this_arg);

    return ecma_make_uint32_value (ecma_typedarray_get_length (typedarray_p));
  }

  return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a TypedArray."));
} /* ecma_builtin_typedarray_prototype_length_getter */

/**
 * @}
 * @}
 * @}
 */

#endif /* !CONFIG_DISABLE_TYPEDARRAY_BUILTIN */
