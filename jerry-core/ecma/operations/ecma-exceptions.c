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

#include <stdarg.h>
#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-gc.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-objects.h"
#include "jcontext.h"
#include "jrt.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup exceptions Exceptions
 * @{
 */

/**
 * Standard ecma-error object constructor.
 *
 * @return pointer to ecma-object representing specified error
 *         with reference counter set to one.
 */
ecma_object_t *
ecma_new_standard_error (ecma_standard_error_t error_type) /**< native error type */
{
#ifndef CONFIG_DISABLE_ERROR_BUILTINS
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
      prototype_id = ECMA_BUILTIN_ID_SYNTAX_ERROR_PROTOTYPE;
      break;
    }
  }
#else
  JERRY_UNUSED (error_type);
  ecma_builtin_id_t prototype_id = ECMA_BUILTIN_ID_ERROR_PROTOTYPE;
#endif /* !CONFIG_DISABLE_ERROR_BUILTINS */

  ecma_object_t *prototype_obj_p = ecma_builtin_get (prototype_id);

  ecma_object_t *new_error_obj_p = ecma_create_object (prototype_obj_p,
                                                       sizeof (ecma_extended_object_t),
                                                       ECMA_OBJECT_TYPE_CLASS);

  ecma_deref_object (prototype_obj_p);

  ((ecma_extended_object_t *) new_error_obj_p)->u.class_prop.class_id = LIT_MAGIC_STRING_ERROR_UL;

  return new_error_obj_p;
} /* ecma_new_standard_error */

/**
 * Standard ecma-error object constructor.
 *
 * @return pointer to ecma-object representing specified error
 *         with reference counter set to one.
 */
ecma_object_t *
ecma_new_standard_error_with_message (ecma_standard_error_t error_type, /**< native error type */
                                      ecma_string_t *message_string_p) /**< message string */
{
  ecma_object_t *new_error_obj_p = ecma_new_standard_error (error_type);

  ecma_string_t *message_magic_string_p = ecma_get_magic_string (LIT_MAGIC_STRING_MESSAGE);

  ecma_property_value_t *prop_value_p;
  prop_value_p = ecma_create_named_data_property (new_error_obj_p,
                                                  message_magic_string_p,
                                                  ECMA_PROPERTY_CONFIGURABLE_WRITABLE,
                                                  NULL);
  ecma_deref_ecma_string (message_magic_string_p);

  ecma_ref_ecma_string (message_string_p);
  prop_value_p->value = ecma_make_string_value (message_string_p);

  return new_error_obj_p;
} /* ecma_new_standard_error_with_message */

/**
 * Raise a standard ecma-error with the given type and message.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_raise_standard_error (ecma_standard_error_t error_type, /**< error type */
                           const lit_utf8_byte_t *msg_p) /**< error message */
{
  ecma_object_t *error_obj_p;

  if (msg_p != NULL)
  {
    ecma_string_t *error_msg_p = ecma_new_ecma_string_from_utf8 (msg_p,
                                                                 lit_zt_utf8_string_size (msg_p));
    error_obj_p = ecma_new_standard_error_with_message (error_type, error_msg_p);
    ecma_deref_ecma_string (error_msg_p);
  }
  else
  {
    error_obj_p = ecma_new_standard_error (error_type);
  }

  JERRY_CONTEXT (error_value) = ecma_make_object_value (error_obj_p);
  return ECMA_VALUE_ERROR;
} /* ecma_raise_standard_error */

#ifdef JERRY_ENABLE_ERROR_MESSAGES

/**
 * Raise a standard ecma-error with the given format string and arguments.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_raise_standard_error_with_format (ecma_standard_error_t error_type, /**< error type */
                                       const char *format, /**< format string */
                                       ...) /**< ecma-values */
{
  JERRY_ASSERT (format != NULL);

  ecma_string_t *error_msg_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);

  const char *start_p = format;
  const char *end_p = format;

  va_list args;

  va_start (args, format);

  while (*end_p)
  {
    if (*end_p == '%')
    {
      /* Concat template string. */
      if (end_p > start_p)
      {
        const lit_utf8_byte_t *chars_p = (const lit_utf8_byte_t *) start_p;
        lit_utf8_size_t chars_size = (lit_utf8_size_t) (end_p - start_p);

        error_msg_p = ecma_append_chars_to_string (error_msg_p,
                                                   chars_p,
                                                   chars_size,
                                                   lit_utf8_string_length (chars_p, chars_size));
      }

      /* Convert an argument to string without side effects. */
      ecma_string_t *arg_string_p;
      const ecma_value_t arg_val = va_arg (args, ecma_value_t);
      if (unlikely (ecma_is_value_object (arg_val)))
      {
        ecma_object_t *arg_object_p = ecma_get_object_from_value (arg_val);
        lit_magic_string_id_t class_name = ecma_object_get_class_name (arg_object_p);
        arg_string_p = ecma_get_magic_string (class_name);
      }
      else
      {
        ecma_value_t str_val = ecma_op_to_string (arg_val);
        arg_string_p = ecma_get_string_from_value (str_val);
      }

      /* Concat argument. */
      error_msg_p = ecma_concat_ecma_strings (error_msg_p, arg_string_p);
      ecma_deref_ecma_string (arg_string_p);

      start_p = end_p + 1;
    }

    end_p++;
  }

  va_end (args);

  /* Concat reset of template string. */
  if (start_p < end_p)
  {
    const lit_utf8_byte_t *chars_p = (const lit_utf8_byte_t *) start_p;
    lit_utf8_size_t chars_size = (lit_utf8_size_t) (end_p - start_p);

    error_msg_p = ecma_append_chars_to_string (error_msg_p,
                                               chars_p,
                                               chars_size,
                                               lit_utf8_string_length (chars_p, chars_size));
  }

  ecma_object_t *error_obj_p = ecma_new_standard_error_with_message (error_type, error_msg_p);
  ecma_deref_ecma_string (error_msg_p);

  JERRY_CONTEXT (error_value) = ecma_make_object_value (error_obj_p);
  return ECMA_VALUE_ERROR;
} /* ecma_raise_standard_error_with_format */

#endif /* JERRY_ENABLE_ERROR_MESSAGES */

/**
 * Raise a common error with the given message.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_raise_common_error (const char *msg_p) /**< error message */
{
  return ecma_raise_standard_error (ECMA_ERROR_COMMON, (const lit_utf8_byte_t *) msg_p);
} /* ecma_raise_common_error */

/**
 * Raise an EvalError with the given message.
 *
 * See also: ECMA-262 v5, 15.11.6.1
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_raise_eval_error (const char *msg_p) /**< error message */
{
  return ecma_raise_standard_error (ECMA_ERROR_EVAL, (const lit_utf8_byte_t *) msg_p);
} /* ecma_raise_eval_error */

/**
 * Raise a RangeError with the given message.
 *
 * See also: ECMA-262 v5, 15.11.6.2
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_raise_range_error (const char *msg_p) /**< error message */
{
  return ecma_raise_standard_error (ECMA_ERROR_RANGE, (const lit_utf8_byte_t *) msg_p);
} /* ecma_raise_range_error */

/**
 * Raise a ReferenceError with the given message.
 *
 * See also: ECMA-262 v5, 15.11.6.3
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_raise_reference_error (const char *msg_p) /**< error message */
{
  return ecma_raise_standard_error (ECMA_ERROR_REFERENCE, (const lit_utf8_byte_t *) msg_p);
} /* ecma_raise_reference_error */

/**
 * Raise a SyntaxError with the given message.
 *
 * See also: ECMA-262 v5, 15.11.6.4
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_raise_syntax_error (const char *msg_p) /**< error message */
{
  return ecma_raise_standard_error (ECMA_ERROR_SYNTAX, (const lit_utf8_byte_t *) msg_p);
} /* ecma_raise_syntax_error */

/**
 * Raise a TypeError with the given message.
 *
* See also: ECMA-262 v5, 15.11.6.5
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_raise_type_error (const char *msg_p) /**< error message */
{
  return ecma_raise_standard_error (ECMA_ERROR_TYPE, (const lit_utf8_byte_t *) msg_p);
} /* ecma_raise_type_error */

/**
 * Raise a URIError with the given message.
 *
* See also: ECMA-262 v5, 15.11.6.6
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value
 */
ecma_value_t
ecma_raise_uri_error (const char *msg_p) /**< error message */
{
  return ecma_raise_standard_error (ECMA_ERROR_URI, (const lit_utf8_byte_t *) msg_p);
} /* ecma_raise_uri_error */

/**
 * @}
 * @}
 */
