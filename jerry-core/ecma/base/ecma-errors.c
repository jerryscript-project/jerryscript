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

#include "ecma-errors.h"

#include "ecma-globals.h"
#include "ecma-helpers.h"

#if JERRY_ERROR_MESSAGES
/**
 * Struct to store ecma error message with its size.
 */
typedef struct
{
  char *text; /* Text of ecma error message. */
  uint8_t size; /* Size of ecma error message. */
} ecma_error_message_t;

/* Error message texts with size. */
static ecma_error_message_t ecma_error_messages[] JERRY_ATTR_CONST_DATA = {
  { "", 0 }, /* ECMA_ERR_EMPTY */
/** @cond doxygen_suppress */
#define ECMA_ERROR_DEF(id, string) { string, sizeof (string) - 1 },
#include "ecma-error-messages.inc.h"
#undef ECMA_ERROR_DEF
  /** @endcond */
};
#endif /* JERRY_ERROR_MESSAGES */

/**
 * Get error string of specified ecma error
 *
 * @return created string for ecma error
 */
ecma_value_t
ecma_get_error_msg (ecma_error_msg_t id) /**< ecma error id */
{
  JERRY_ASSERT (id != ECMA_IS_VALID_CONSTRUCTOR);
  ecma_string_t *ecma_str_p;
#if JERRY_ERROR_MESSAGES
  ecma_error_message_t *msg = ecma_error_messages + id;
  JERRY_ASSERT (lit_is_valid_cesu8_string ((const lit_utf8_byte_t *) msg->text, msg->size));
  ecma_str_p = ecma_new_ecma_string_from_ascii ((const lit_utf8_byte_t *) msg->text, msg->size);
#else /* !JERRY_ERROR_MESSAGES */
  ecma_str_p = ecma_get_magic_string (LIT_MAGIC_STRING__EMPTY);
#endif /* JERRY_ERROR_MESSAGES */
  return ecma_make_string_value (ecma_str_p);
} /* ecma_get_error_msg */
