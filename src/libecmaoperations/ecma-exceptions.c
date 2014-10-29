/* Copyright 2014 Samsung Electronics Co., Ltd.
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

#include "ecma-builtins.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "globals.h"

/** \addtogroup ecma ECMA
 * @{
 */

/**
 * \addtogroup exceptions Exceptions
 * @{
 */

/**
 * Standard ecma-error object constructor.
 *
 * @return pointer to ecma-object representing specified error
 *         with reference counter set to one.
 */
ecma_object_t*
ecma_new_standard_error (ecma_standard_error_t error_type) /**< native error type */
{
  ecma_builtin_id_t prototype_id = ECMA_BUILTIN_ID__COUNT;

  switch (error_type)
  {
    case ECMA_ERROR_COMMON:
    {
      prototype_id = ECMA_BUILTIN_ID_ERROR_PROTOTYPE;
      break;
    }

    case ECMA_ERROR_EVAL:
    {
      prototype_id = ECMA_BUILTIN_ID_EVAL_ERROR_PROTOTYPE;
      break;
    }

    case ECMA_ERROR_RANGE:
    {
      prototype_id = ECMA_BUILTIN_ID_RANGE_ERROR_PROTOTYPE;
      break;
    }

    case ECMA_ERROR_REFERENCE:
    {
      prototype_id = ECMA_BUILTIN_ID_REFERENCE_ERROR_PROTOTYPE;
      break;
    }

    case ECMA_ERROR_TYPE:
    {
      prototype_id = ECMA_BUILTIN_ID_TYPE_ERROR_PROTOTYPE;
      break;
    }

    case ECMA_ERROR_URI:
    {
      prototype_id = ECMA_BUILTIN_ID_URI_ERROR_PROTOTYPE;
      break;
    }

    case ECMA_ERROR_SYNTAX:
    {
      /* SyntaxError should be treated as an early error */
      JERRY_UNREACHABLE ();
    }
  }

  ecma_object_t *prototype_obj_p = ecma_builtin_get (prototype_id);

  ecma_object_t *new_error_obj_p = ecma_create_object (prototype_obj_p,
                                                       true,
                                                       ECMA_OBJECT_TYPE_GENERAL);

  ecma_deref_object (prototype_obj_p);

  ecma_property_t *class_prop_p = ecma_create_internal_property (new_error_obj_p,
                                                                 ECMA_INTERNAL_PROPERTY_CLASS);
  class_prop_p->u.internal_property.value = ECMA_MAGIC_STRING_ERROR_UL;

  return new_error_obj_p;
} /* ecma_new_standard_error */

/**
 * Standard ecma-error object constructor.
 *
 * @return pointer to ecma-object representing specified error
 *         with reference counter set to one.
 */
ecma_object_t*
ecma_new_standard_error_with_message (ecma_standard_error_t error_type, /**< native error type */
                                      ecma_string_t* message_string_p) /**< message string */
{
  ecma_object_t *new_error_obj_p = ecma_new_standard_error (error_type);

  ecma_string_t *message_magic_string_p = ecma_get_magic_string (ECMA_MAGIC_STRING_MESSAGE);
  ecma_property_t *prop_p = ecma_create_named_data_property (new_error_obj_p,
                                                             message_magic_string_p,
                                                             ECMA_PROPERTY_WRITABLE,
                                                             ECMA_PROPERTY_NOT_ENUMERABLE,
                                                             ECMA_PROPERTY_CONFIGURABLE);

  ecma_string_t *msg_string_p = ecma_copy_or_ref_ecma_string (message_string_p);
  prop_p->u.named_data_property.value = ecma_make_string_value (msg_string_p);
  ecma_deref_ecma_string (message_magic_string_p);

  return new_error_obj_p;
} /* ecma_new_standard_error_with_message */

/**
 * @}
 * @}
 */
