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

#include "jcontext.h"
#include "jerryscript.h"
#include "jrt.h"
#include "lit-strings.h"

#include <emscripten/emscripten.h>
#include <string.h>
#include <stdlib.h>

#define TYPE_ERROR \
    jerry_create_error (JERRY_ERROR_TYPE, NULL);

#define TYPE_ERROR_ARG \
    jerry_create_error ( \
      JERRY_ERROR_TYPE, (const jerry_char_t *)"wrong type of argument");

#define TYPE_ERROR_FLAG \
    jerry_create_error ( \
        JERRY_ERROR_TYPE, \
        (const jerry_char_t *) "argument cannot have an error flag");

static jerry_value_t
jerry_get_arg_value (jerry_value_t value) /**< return value */
{
  return (jerry_value_t) EM_ASM_INT (
    {
      return __jerry.getArgValueRef ($0);
    }
    , value);
} /* jerry_get_arg_value */

/***************************
 * Parser and Executor Function
 *************************/

/**
 * Perform eval
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return result of eval, may be error value.
 */
jerry_value_t
jerry_eval (const jerry_char_t *source_p, /**< source code */
            size_t source_size, /**< length of source code */
            bool is_strict) /**< source must conform with strict mode */
{
  return (jerry_value_t) EM_ASM_INT (
    {
      /* jerry_eval () uses an indirect eval () call,
       * so the global execution context is used.
       * Also see ECMA 5.1 -- 10.4.2 Entering Eval Code.*/
      var indirectEval = eval;
      try
      {
        var source = Module.Pointer_stringify ($0, $1);
        var strictComment = __jerry.getUseStrictComment ($2);
        return __jerry.ref (indirectEval (strictComment + source));
      }
      catch (e)
      {
        return __jerry.setErrorByValue (e);
      }
    }
    , source_p, source_size, is_strict);
} /* jerry_eval */

/**
 * Run garbage collection
 */
void
jerry_gc (void)
{
  EM_ASM (
    {
      /* Hint: use `node --expose-gc` to enable this! */
      if (typeof gc === 'function')
      {
        gc ();
      }
    }
  );
} /* jerry_gc */

/**
 * Parse script and construct an EcmaScript function. The lexical
 * environment is set to the global lexical environment.
 *
 * @return function object value - if script was parsed successfully,
 *         thrown error - otherwise
 */
jerry_value_t
jerry_parse (const jerry_char_t *source_p, /**< script source */
             size_t source_size, /**< script source size */
             bool is_strict) /**< strict mode */
{
  return (jerry_value_t) EM_ASM_INT (
    {
      var source = Module.Pointer_stringify ($0, $1);
      var strictComment = __jerry.getUseStrictComment ($2);
      var strictCommentAndSource = strictComment + source;
      try
      {
        /* Use new Function just to parse the source
         * and immediately throw any syntax errors if needed. */
        new Function (strictCommentAndSource);
        /* If it parsed OK, use a function with a wrapped, indirect eval call
         * execute it later on when jerry_run is called.
         * Indirect eval is used because the global execution context must be
         * used when running the source to mirror the behavior of jerry_parse. */
        var f = function ()
        {
          var indirectEval = eval;
          return indirectEval (strictCommentAndSource);
        };
        return __jerry.ref (f);
      }
      catch (e)
      {
        return __jerry.setErrorByValue (e);
      }
    }
    , source_p, source_size, is_strict);
} /* jerry_parse */

/**
 * Parse function and construct an EcmaScript function. The lexical
 * environment is set to the global lexical environment.
 *
 * @return function object value - if script was parsed successfully,
 *         thrown error - otherwise
 */
jerry_value_t
jerry_parse_function (const jerry_char_t *resource_name_p, /**< resource name (usually a file name) */
                      size_t resource_name_length, /**< length of resource name */
                      const jerry_char_t *arg_list_p, /**< script source */
                      size_t arg_list_size, /**< script source size */
                      const jerry_char_t *source_p, /**< script source */
                      size_t source_size, /**< script source size */
                      bool is_strict) /**< strict mode */
{
  JERRY_UNUSED (resource_name_p);
  JERRY_UNUSED (resource_name_length);
  return (jerry_value_t) EM_ASM_INT (
    {
      var args = Module.Pointer_stringify ($0, $1);
      var source = Module.Pointer_stringify ($2, $3);
      var strictComment = __jerry.getUseStrictComment ($4);
      var strictCommentAndSource = strictComment + source;
      try
      {
        new Function (strictCommentAndSource);
        var funcStr = "(function (" + args + "){" + strictCommentAndSource + "})";
        var indirectEval = eval;
        var f = indirectEval (funcStr);
        return __jerry.ref (f);
      }
      catch (e)
      {
        return __jerry.setErrorByValue (e);
      }
    }
    , arg_list_p, arg_list_size, source_p, source_size, is_strict);
} /* jerry_parse_function */

/**
 * Parse script and construct an ECMAScript function. The lexical
 * environment is set to the global lexical environment. The name
 * (usually a file name) is also passed to this function which is
 * used by the debugger to find the source code.
 *
 * @return function object value - if script was parsed successfully,
 *         thrown error - otherwise
 */
jerry_value_t
jerry_parse_named_resource (const jerry_char_t *name_p, /**< name (usually a file name) */
                            size_t name_length, /**< length of name */
                            const jerry_char_t *source_p, /**< script source */
                            size_t source_size, /**< script source size */
                            bool is_strict) /**< strict mode */
{
  JERRY_UNUSED (name_p);
  JERRY_UNUSED (name_length);
  return jerry_parse (source_p, source_size, is_strict);
} /* jerry_parse_named_resource */

/**
 * Run an EcmaScript function created by jerry_parse.
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return result of bytecode - if run was successful
 *         thrown error - otherwise
 */
jerry_value_t
jerry_run (const jerry_value_t func_val) /**< function to run */
{
  return (jerry_value_t) EM_ASM_INT (
    {
      var f = __jerry.get ($0);
      try
      {
        if (typeof f !== 'function')
        {
          throw new Error ('wrong type of argument');
        }
        var result = f ();
        return __jerry.ref (result);
      }
      catch (e)
      {
        return __jerry.setErrorByValue (e);
      };
    }
    , func_val);
} /* jerry_run */

/**
 * Simple Jerry runner
 *
 * @return true  - if run was successful
 *         false - otherwise
 */
bool
jerry_run_simple (const jerry_char_t *script_source_p, /**< script source */
                  size_t script_source_size, /**< script source size */
                  jerry_init_flag_t flags) /**< combination of Jerry flags */
{
  jerry_init (flags);
  const jerry_value_t eval_ret_val = jerry_eval (script_source_p, script_source_size, false /* is_strict */);
  const bool has_error = jerry_value_has_error_flag (eval_ret_val);
  jerry_release_value (eval_ret_val);
  jerry_cleanup ();
  return !has_error;
} /* jerry_run_simple */

/**
 * Acquire specified Jerry API value.
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return acquired api value
 */
jerry_value_t
jerry_acquire_value (jerry_value_t value) /**< API value */
{
  return (jerry_value_t) EM_ASM_INT (
    {
      return __jerry.acquire ($0);
    }
    , value);
} /* jerry_acquire_value */

/**
 * Release specified Jerry API value
 */
void jerry_release_value (jerry_value_t value) /**< API value */
{
  EM_ASM_INT (
    {
      __jerry.release ($0);
    }
    , value);
} /* jerry_release_value */

/**
 * Run enqueued Promise jobs until the first thrown error or until all get executed.
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return result of last executed job, may be error value.
 */
jerry_value_t
jerry_run_all_enqueued_jobs (void)
{
  return (jerry_value_t) EM_ASM_INT_V (
    {
      return __jerry.runJobQueue ();
    }
  );
} /* jerry_run_all_enqueued_jobs */

/******************
 *  Get the global context
 ******************/

/**
 * Get global object
 *
 * Note:
 *      returned value must be freed with jerry_release_value, when it is no longer needed.
 *
 * @return api value of global object
 */
jerry_value_t
jerry_get_global_object (void)
{
  return (jerry_value_t) EM_ASM_INT_V (
    {
      return __jerry.ref (Function ('return this;') ());
    }
  );
} /* jerry_get_global_object */

/*********************
 * Jerry Value Type Checking
 ************************/

#define JERRY_VALUE_HAS_TYPE(ref, typename) \
    ((bool) EM_ASM_INT ({ \
             return typeof __jerry.get ($0) === (typename); \
           }, (ref)))

#define JERRY_VALUE_IS_INSTANCE(ref, type) \
    ((bool) EM_ASM_INT ({ \
             return __jerry.get ($0) instanceof (type); \
           }, (ref)))

bool jerry_value_is_array (const jerry_value_t val)
{
  jerry_value_t value = jerry_get_arg_value (val);
  return JERRY_VALUE_IS_INSTANCE (value, Array);
} /* jerry_value_is_array */

/**
 * Check if the specified value is boolean.
 *
 * @return true  - if the specified value is boolean,
 *         false - otherwise
 */
bool jerry_value_is_boolean (const jerry_value_t val) /**< api value */
{
  jerry_value_t value = jerry_get_arg_value (val);
  return JERRY_VALUE_HAS_TYPE (value, 'boolean');
} /* jerry_value_is_boolean */

/**
 * Check if the specified value is a constructor function object value.
 *
 * @return true - if the specified value is a function value that implements [[Construct]],
 *         false - otherwise
 */
bool jerry_value_is_constructor (const jerry_value_t val) /**< api value */
{
  jerry_value_t value = jerry_get_arg_value (val);
  return jerry_value_is_function (value);
} /* jerry_value_is_constructor */

/**
 * Check if the specified value is a function object value.
 *
 * @return true - if the specified value is callable,
 *         false - otherwise
 */
bool jerry_value_is_function (const jerry_value_t val) /**< api value */
{
  jerry_value_t value = jerry_get_arg_value (val);
  return JERRY_VALUE_HAS_TYPE (value, 'function');
} /* jerry_value_is_function */

/**
 * Check if the specified value is number.
 *
 * @return true  - if the specified value is number,
 *         false - otherwise
 */
bool jerry_value_is_number (const jerry_value_t val) /**< api value */
{
  jerry_value_t value = jerry_get_arg_value (val);
  return JERRY_VALUE_HAS_TYPE (value, 'number');
} /* jerry_value_is_number */

/**
 * Check if the specified value is null.
 *
 * @return true  - if the specified value is null,
 *         false - otherwise
 */
bool jerry_value_is_null (const jerry_value_t val) /**< api value */
{
  jerry_value_t value = jerry_get_arg_value (val);
  return ((bool) EM_ASM_INT (
    {
      return __jerry.get ($0) === null;
    }
    , value));
} /* jerry_value_is_null */

/**
 * Check if the specified value is object.
 *
 * @return true  - if the specified value is object,
 *         false - otherwise
 */
bool jerry_value_is_object (const jerry_value_t val) /**< api value */
{
  jerry_value_t value = jerry_get_arg_value (val);
  return (bool) EM_ASM_INT (
    {
      var value = __jerry.get ($0);
      if (value === null)
      {
        return false;
      }
      var typeStr = typeof value;
      return typeStr === 'object' || typeStr === 'function';
    }
    , value);
} /* jerry_value_is_object */

/**
 * Check if the specified value is string.
 *
 * @return true  - if the specified value is string,
 *         false - otherwise
 */
bool jerry_value_is_string (const jerry_value_t val) /**< api value */
{
  jerry_value_t value = jerry_get_arg_value (val);
  return JERRY_VALUE_HAS_TYPE (value, 'string');
} /* jerry_value_is_string */


/**
 * Check if the specified value is undefined.
 *
 * @return true  - if the specified value is undefined,
 *         false - otherwise
 */
bool jerry_value_is_undefined (const jerry_value_t val) /**< api value */
{
  jerry_value_t value = jerry_get_arg_value (val);
  return JERRY_VALUE_HAS_TYPE (value, 'undefined');
} /* jerry_value_is_undefined */

/**
 * Check if the specified value is promise.
 *
 * @return true  - if the specified value is promise,
 *         false - otherwise
 */
bool
jerry_value_is_promise (const jerry_value_t val) /**< api value */
{
  jerry_value_t value = jerry_get_arg_value (val);
  return (bool) EM_ASM_INT (
    {
      var value = __jerry.get ($0);
      return value instanceof Module.Promise;
    }
    , value);
} /* jerry_value_is_promise */

/*********************************
 * Jerry Value Getter Functions
 ********************************/

/**
 * Get boolean from the specified value.
 *
 * @return true or false.
 */
bool jerry_get_boolean_value (const jerry_value_t val) /**< api value */
{
  jerry_value_t value = jerry_get_arg_value (val);
  if (!jerry_value_is_boolean (value))
  {
    return false;
  }
  return (bool) EM_ASM_INT (
    {
      return (__jerry.get ($0) === true);
    }
    , value);
} /* jerry_get_boolean_value */

/**
 * Get number from the specified value as a double.
 *
 * @return stored number as double
 */
double jerry_get_number_value (const jerry_value_t val) /**< api value */
{
  jerry_value_t value = jerry_get_arg_value (val);
  if (!jerry_value_is_number (value))
  {
    return 0.0;
  }
  return EM_ASM_DOUBLE (
    {
      return __jerry.get ($0);
    }
    , value);
} /* jerry_get_number_value */

/************************************
 * Functions for UTF-8 encoded string values
 * JerryScript's internal string encoding is CESU-8.
 ***************************/

/**
 * Get UTF-8 encoded string size from Jerry string
 *
 * Note:
 *      Returns 0, if the value parameter is not a string.
 *
 * @return number of bytes in the buffer needed to represent the UTF-8 encoded string
 */
jerry_size_t jerry_get_utf8_string_size (const jerry_value_t val) /**< api value */
{
  jerry_value_t value = jerry_get_arg_value (val);
  if (!jerry_value_is_string (value))
  {
    return 0;
  }
  return (jerry_size_t) EM_ASM_INT (
    {
      return Module.lengthBytesUTF8 (__jerry.get ($0));
    }
    , value);
} /* jerry_get_utf8_string_size */

/**
 * Copy the characters of an utf-8 encoded string into a specified buffer.
 *
 * Note:
 *      The '\0' character could occur anywhere in the returned string
 *      Returns 0, if the value parameter is not a string or the buffer
 *      is not large enough for the whole string.
 *
 * @return number of bytes copied to the buffer.
 */
jerry_size_t
jerry_string_to_utf8_char_buffer (const jerry_value_t val, /**< input string value */
                                  jerry_char_t *buffer_p, /**< [out] output characters buffer */
                                  jerry_size_t buffer_size) /**< size of output buffer */
{
  jerry_value_t value = jerry_get_arg_value (val);
  const jerry_size_t str_size = jerry_get_utf8_string_size (value);
  if (str_size == 0 || buffer_size < str_size || buffer_p == NULL)
  {
    return 0;
  }

  return (jerry_size_t) EM_ASM_INT (
    {
      var str = __jerry.get ($0);
      return Module.stringToUTF8DataOnly (str, $1, $2);
    }
    , value, buffer_p, buffer_size);
} /* jerry_string_to_utf8_char_buffer */

/**
 * Copy the characters of an cesu-8 encoded substring into a specified buffer.
 *
 * Note:
 *      The '\0' character could occur anywhere in the returned string
 *      Returns 0, if the value parameter is not a string.
 *      It will extract the substring beetween the specified start position
 *      and the end position (or the end of the string, whichever comes first).
 *
 * @return number of bytes copied to the buffer.
 */
jerry_size_t
jerry_substring_to_char_buffer (const jerry_value_t val, /**< input string value */
                                jerry_length_t start_pos, /**< position of the first character */
                                jerry_length_t end_pos, /**< position of the last character */
                                jerry_char_t *buffer_p, /**< [out] output characters buffer */
                                jerry_size_t buffer_size) /**< size of output buffer */
{
  jerry_value_t value = jerry_get_arg_value (val);
  if (buffer_p == NULL || buffer_size == 0 || !jerry_value_is_string (value))
  {
    return 0;
  }
  return (jerry_size_t) EM_ASM_INT (
    {
      var str = __jerry.get ($0);
      var substr = str.slice ($1, $2);
      return Module.stringToCESU8DataOnly (substr, $3, $4);
    }
    , value, start_pos, end_pos, buffer_p, buffer_size);
} /* jerry_substring_to_char_buffer */

/**
 * Validate UTF-8 string.
 *
 * @return true - if UTF-8 string is well-formed
 *         false - otherwise
 */
bool
jerry_is_valid_utf8_string (const jerry_char_t *utf8_buf_p, /**< UTF-8 string */
                            jerry_size_t buf_size) /**< string size */
{
  return lit_is_valid_utf8_string ((lit_utf8_byte_t *) utf8_buf_p,
                                   (lit_utf8_size_t) buf_size);
} /* jerry_is_valid_utf8_string */

/**
 * Validate CESU-8 string.
 *
 * @return true - if CESU-8 string is well-formed
 *         false - otherwise
 */
bool
jerry_is_valid_cesu8_string (const jerry_char_t *cesu8_buf_p, /**< CESU-8 string */
                             jerry_size_t buf_size) /**< string size */
{
  return lit_is_valid_cesu8_string ((lit_utf8_byte_t *) cesu8_buf_p,
                                    (lit_utf8_size_t) buf_size);
} /* jerry_is_valid_cesu8_string */

/**
 * Get UTF-8 string length from Jerry string
 *
 * Note:
 *      Returns 0, if the value parameter is not a string.
 *
 * @return number of characters in the string
 */
jerry_length_t
jerry_get_utf8_string_length (const jerry_value_t val) /**< input string */
{
  jerry_value_t value = jerry_get_arg_value (val);
  if (!jerry_value_is_string (value))
  {
    return 0;
  }
  return (jerry_length_t) EM_ASM_INT (
    {
      var str = __jerry.get ($0);
      var utf8Length = str.length;
      for (var i = 0; i < str.length; ++i)
      {
        var utf16 = str.charCodeAt (i);
        if (utf16 >= 0xD800 && utf16 <= 0xDFFF)
        {
          /* Lead surrogate code point. */
          --utf8Length;
          ++i;
        }
      }
      return utf8Length;
    }
    , value);
} /* jerry_get_utf8_string_length */

/**
 * Copy the characters of a string into a specified buffer.
 *
 * Note:
 *      The '\0' character could occur in character buffer.
 *      Returns 0, if the value parameter is not a string or
 *      the buffer is not large enough for the whole string.
 *
 * @return number of bytes, actually copied to the buffer.
 */
jerry_size_t
jerry_substring_to_utf8_char_buffer (const jerry_value_t val, /**< input string value */
                                     jerry_length_t start_pos, /**< position of the first character */
                                     jerry_length_t end_pos, /**< position of the last character */
                                     jerry_char_t *buffer_p, /**< [out] output characters buffer */
                                     jerry_size_t buffer_size) /**< size of output buffer */
{
  jerry_value_t value = jerry_get_arg_value (val);
  if (buffer_p == NULL || buffer_size == 0 || !jerry_value_is_string (value))
  {
    return 0;
  }
  return (jerry_size_t) EM_ASM_INT (
    {
      var str = __jerry.get ($0);
      /* String.prototype.slice ()'s beginIndex/endIndex arguments aren't
       * Unicode codepoint positions: surrogates are counted separately... */
      var utf8Pos = 0;
      var utf16StartPos;
      var utf16EndPos = str.length;
      for (var i = 0; i < str.length; ++i)
      {
        if (utf8Pos === $1)
        {
          utf16StartPos = i;
        }
        ++utf8Pos;
        var utf16 = str.charCodeAt (i);
        if (utf16 >= 0xD800 && utf16 <= 0xDFFF)
        {
          // Lead surrogate code point
          ++i; // Skip over the trailing surrogate
        }
        if (utf8Pos === $2)
        {
          utf16EndPos = i;
          break;
        }
      }
      if (utf16StartPos === undefined)
      {
        return 0;
      }
      var substr = str.slice (utf16StartPos, utf16EndPos + 1);
      console.log (utf16StartPos, utf16EndPos, substr.length, str.length);
      return Module.stringToUTF8DataOnly (substr, $3, $4);
    }
    , value, start_pos, end_pos, buffer_p, buffer_size);
} /* jerry_substring_to_utf8_char_buffer */

/**
 * Create string from a valid UTF-8 string
 *
 * Note:
 *      returned value must be freed with jerry_release_value when it is no longer needed.
 *
 * @return value of the created string
 */
jerry_value_t
jerry_create_string_from_utf8 (const jerry_char_t *str_p) /**< pointer to string */
{
  // Just call jerry_create_string_sz, it auto-detects UTF-8.
  return jerry_create_string_sz (str_p, strlen ((const char *) str_p));
} /* jerry_create_string_from_utf8 */

/**
 * Create string from a valid UTF-8 string
 *
 * Note:
 *      returned value must be freed with jerry_release_value when it is no longer needed.
 *
 * @return value of the created string
 */
jerry_value_t
jerry_create_string_sz_from_utf8 (const jerry_char_t *str_p, /**< pointer to string */
                                  jerry_size_t str_size) /**< string size */
{
  // Just call jerry_create_string_sz, it auto-detects UTF-8.
  return jerry_create_string_sz (str_p, str_size);
} /* jerry_create_string_sz_from_utf8 */

/**
 * Create string from a valid CESU-8 string
 *
 * Note:
 *      returned value must be freed with jerry_release_value when it is no longer needed.
 *
 * @return value of the created string
 */
jerry_value_t
jerry_create_string_sz (const jerry_char_t *str_p, /**< pointer to string */
                        jerry_size_t str_size) /**< string size */
{
  if (!str_p)
  {
    return jerry_create_undefined ();
  }
  return (jerry_value_t) EM_ASM_INT (
    {
      /* Auto-detects ASCII vs UTF-8: */
      return __jerry.ref (Module.Pointer_stringify ($0, $1));
    }
    , str_p, str_size);
} /* jerry_create_string_sz */

/**
 * Creates a jerry_value_t representing an undefined value.
 *
 * @return value of undefined
 */
jerry_value_t jerry_create_string (const jerry_char_t *str_p)
{
  if (!str_p)
  {
    return jerry_create_undefined ();
  }
  return jerry_create_string_sz (str_p, strlen ((const char *) str_p));
} /* jerry_create_string */

/**
 * Get length of an array object
 *
 * Note:
 *      Returns 0, if the value parameter is not an array object.
 *
 * @return length of the given array
 */
jerry_size_t jerry_get_string_size (const jerry_value_t val)
{
  jerry_value_t value = jerry_get_arg_value (val);
  if (!jerry_value_is_string (value))
  {
    return 0;
  }
  return (jerry_size_t) EM_ASM_INT (
    {
      var str = __jerry.get ($0);
      var cesu8Size = 0;
      for (var i = 0; i < str.length; ++i)
      {
        var utf16 = str.charCodeAt (i);
        if (utf16 <= 0x7F)
        {
          ++cesu8Size;
        }
        else if (utf16 <= 0x7FF)
        {
          cesu8Size += 2;
        }
        else if (utf16 <= 0xFFFF)
        {
          cesu8Size += 3;
        }
      }
      return cesu8Size;
    }
    , value);
} /* jerry_get_string_size */

/**
 * Register external magic string array
 */
void
jerry_register_magic_strings (const jerry_char_ptr_t *ex_str_items_p, /**< character arrays, representing
                                                                       *   external magic strings' contents */
                              uint32_t count, /**< number of the strings */
                              const jerry_length_t *str_lengths_p) /**< lengths of all strings */
{
  /* No-op. This is part of an internal implementation detail to optimize string performance. */
  JERRY_UNUSED (ex_str_items_p);
  JERRY_UNUSED (count);
  JERRY_UNUSED (str_lengths_p);
} /* jerry_register_magic_strings */

/*************************
 * Functions for array object values
 **********************/

uint32_t jerry_get_array_length (const jerry_value_t val)
{
  jerry_value_t value = jerry_get_arg_value (val);
  if (!jerry_value_is_array (value))
  {
    return 0;
  }
  return (uint32_t) EM_ASM_INT (
    {
      return __jerry.get ($0).length;
    }
    , value);
} /* jerry_get_array_length */

/****************************
 * Jerry Value Creation API
 ***************************/
jerry_value_t jerry_create_array (uint32_t size)
{
  return (jerry_value_t) EM_ASM_INT (
    {
      return __jerry.ref (new Array ($0));
    }
    , size);
} /* jerry_create_array */

jerry_value_t jerry_create_boolean (bool value)
{
  return (jerry_value_t) EM_ASM_INT (
    {
      return __jerry.ref (Boolean ($0));
    }
    , value);
} /* jerry_create_boolean */

jerry_value_t jerry_create_error (jerry_error_t error_type,
                                  const jerry_char_t *message_p)
{
  return jerry_create_error_sz (error_type,
                                message_p, strlen ((const char *) message_p));
} /* jerry_create_error */

#define JERRY_ERROR(type, msg, sz) (jerry_value_t) (EM_ASM_INT ({ \
    return __jerry.ref (new (type)(Module.Pointer_stringify($0, $1))) \
  }, (msg), (sz)))

jerry_value_t jerry_create_error_sz (jerry_error_t error_type,
                                     const jerry_char_t *message_p,
                                     jerry_size_t message_size)
{
  jerry_value_t error_ref = 0;
  switch (error_type)
  {
    case JERRY_ERROR_COMMON:
    {
      error_ref = JERRY_ERROR (Error, message_p, message_size);
      break;
    }
    case JERRY_ERROR_EVAL:
    {
      error_ref = JERRY_ERROR (EvalError, message_p, message_size);
      break;
    }
    case JERRY_ERROR_RANGE:
    {
      error_ref = JERRY_ERROR (RangeError, message_p, message_size);
      break;
    }
    case JERRY_ERROR_REFERENCE:
    {
      error_ref = JERRY_ERROR (ReferenceError, message_p, message_size);
      break;
    }
    case JERRY_ERROR_SYNTAX:
    {
      error_ref = JERRY_ERROR (SyntaxError, message_p, message_size);
      break;
    }
    case JERRY_ERROR_TYPE:
    {
      error_ref = JERRY_ERROR (TypeError, message_p, message_size);
      break;
    }
    case JERRY_ERROR_URI:
    {
      error_ref = JERRY_ERROR (URIError, message_p, message_size);
      break;
    }
    default:
    {
      EM_ASM_INT (
        {
          abort ('Cannot create error type: ' + $0);
        }
        , error_type);
      break;
    }
  }
  jerry_value_set_error_flag (&error_ref);
  return error_ref;
} /* jerry_create_error_sz */

jerry_value_t jerry_create_external_function (jerry_external_handler_t handler_p)
{
  return (jerry_value_t) EM_ASM_INT (
    {
      return __jerry.create_external_function ($0);
    }
    , handler_p);
} /* jerry_create_external_function */

jerry_value_t jerry_create_number (double value)
{
  return (jerry_value_t) EM_ASM_INT (
    {
      return __jerry.ref ($0);
    }
    , value);
} /* jerry_create_number */

jerry_value_t jerry_create_number_infinity (bool negative)
{
  return (jerry_value_t) EM_ASM_INT (
    {
      return __jerry.ref ($0 ? -Infinity : Infinity);
    }
    , negative);
} /* jerry_create_number_infinity */

jerry_value_t jerry_create_number_nan (void)
{
  return (jerry_value_t) EM_ASM_INT_V (
    {
      return __jerry.ref (NaN);
    }
  );
} /* jerry_create_number_nan */

jerry_value_t jerry_create_null (void)
{
  return (jerry_value_t) EM_ASM_INT_V (
    {
      return __jerry.ref (null);
    }
  );
} /* jerry_create_null */

jerry_value_t jerry_create_object (void)
{
  return (jerry_value_t) EM_ASM_INT_V (
    {
      return __jerry.ref (new Object ());
    }
  );
} /* jerry_create_object */

jerry_value_t jerry_create_undefined (void)
{
  return (jerry_value_t) EM_ASM_INT_V (
    {
      return __jerry.ref (undefined);
    }
  );
} /* jerry_create_undefined */

jerry_value_t jerry_create_promise (void)
{
  return (jerry_value_t) EM_ASM_INT_V (
    {
      /* Save the resolve/reject function in the promise's internal props */
      var resolve_func;
      var reject_func;
      var _Promise = Module.Promise || Promise;
      var p = new _Promise (function (f, r)
        {
          resolve_func = f;
          reject_func = r;
        }
      );
      var ref = __jerry.ref (p);
      var internalProps = __jerry._jerryInternalPropsWeakMap.get (p);
      internalProps.promiseResolveFunc = resolve_func;
      internalProps.promiseRejectFunc = reject_func;
      return ref;
    }
  );
} /* jerry_create_promise */

/***********************************
 * General API Functions of JS Objects
 ***************************************/

jerry_value_t
jerry_has_property (const jerry_value_t obj_value, /**< object value */
                    const jerry_value_t prop_name_value) /**< property name (string value) */
{
  jerry_value_t obj_val = jerry_get_arg_value (obj_value);
  jerry_value_t prop_name_val = jerry_get_arg_value (prop_name_value);
  if (!jerry_value_is_object (obj_val) ||
      !jerry_value_is_string (prop_name_val))
  {
    return false;
  }
  const bool has_property = (bool) EM_ASM_INT (
    {
      var obj = __jerry.get ($0);
      var name = __jerry.get ($1);
      return (name in obj);
    }
    , obj_val, prop_name_val);
  return jerry_create_boolean (has_property);
} /* jerry_has_property */

jerry_value_t
jerry_has_own_property (const jerry_value_t obj_value, /**< object value */
                        const jerry_value_t prop_name_value) /**< property name (string value) */
{
  jerry_value_t obj_val = jerry_get_arg_value (obj_value);
  jerry_value_t prop_name_val = jerry_get_arg_value (prop_name_value);
  if (!jerry_value_is_object (obj_val) ||
      !jerry_value_is_string (prop_name_val))
  {
    return false;
  }
  const bool has_property = (bool) EM_ASM_INT (
    {
      var obj = __jerry.get ($0);
      var name = __jerry.get ($1);
      return obj.hasOwnProperty (name);
    }
    , obj_val, prop_name_val);
  return jerry_create_boolean (has_property);
} /* jerry_has_own_property */

bool jerry_delete_property (const jerry_value_t obj_value,
                            const jerry_value_t prop_name_value)
{
  jerry_value_t obj_val = jerry_get_arg_value (obj_value);
  jerry_value_t prop_name_val = jerry_get_arg_value (prop_name_value);
  if (!jerry_value_is_object (obj_val) ||
      !jerry_value_is_string (prop_name_val))
  {
    return false;
  }
  return (bool) EM_ASM_INT (
    {
      var obj = __jerry.get ($0);
      var name = __jerry.get ($1);
      try
      {
        return delete obj[name];
      }
      catch (e)
      {
        /* In strict mode, delete throws SyntaxError if the property is an
         * own non-configurable property. */
        return false;
      }
    return true;
    }
    , obj_val, prop_name_val);
} /* jerry_delete_property */

bool jerry_delete_property_by_index (const jerry_value_t obj_value, uint32_t index)
{
  jerry_value_t obj_val = jerry_get_arg_value (obj_value);
  if (!jerry_value_is_object (obj_val))
  {
    return false;
  }
  return (bool) EM_ASM_INT (
    {
      var obj = __jerry.get ($0);
      try
      {
        return delete obj[$1];
      }
      catch (e)
      {
        /* In strict mode, delete throws SyntaxError if the property is an
         * own non-configurable property. */
        return false;
      }
    }
    , obj_val, index);
} /* jerry_delete_property_by_index */


jerry_value_t jerry_get_property (const jerry_value_t obj_value,
                                  const jerry_value_t prop_name_value)
{
  jerry_value_t obj_val = jerry_get_arg_value (obj_value);
  jerry_value_t prop_name_val = jerry_get_arg_value (prop_name_value);
  if (!jerry_value_is_object (obj_val) ||
      !jerry_value_is_string (prop_name_val))
  {
    return TYPE_ERROR_ARG;
  }
  return (jerry_value_t) EM_ASM_INT (
    {
      var obj = __jerry.get ($0);
      var name = __jerry.get ($1);
      try
      {
        var rv = obj[name];
      }
      catch (e)
      {
        return __jerry.setErrorByValue (e);
      }
      return __jerry.ref (rv);
    }
    , obj_val, prop_name_val);
} /* jerry_get_property */

jerry_value_t jerry_get_property_by_index (const jerry_value_t obj_value,
                                           uint32_t index)
{
  jerry_value_t obj_val = jerry_get_arg_value (obj_value);
  if (!jerry_value_is_object (obj_val))
  {
    return TYPE_ERROR;
  }
  return (jerry_value_t) EM_ASM_INT (
    {
      var obj = __jerry.get ($0);
      try
      {
        var rv = obj[$1];
      }
      catch (e)
      {
        return __jerry.setErrorByValue (e);
      }
      return __jerry.ref (rv);
    }
    , obj_val, index);
} /* jerry_get_property_by_index */

jerry_value_t jerry_set_property (const jerry_value_t obj_value,
                                  const jerry_value_t prop_name_value,
                                  const jerry_value_t value_to_set)
{
  jerry_value_t obj_val = jerry_get_arg_value (obj_value);
  jerry_value_t prop_name_val = jerry_get_arg_value (prop_name_value);
  if (jerry_value_has_error_flag (value_to_set) ||
      !jerry_value_is_object (obj_val) ||
      !jerry_value_is_string (prop_name_val))
  {
    return TYPE_ERROR_ARG;
  }
  return (jerry_value_t) EM_ASM_INT (
    {
      var obj = __jerry.get ($0);
      var name = __jerry.get ($1);
      var to_set = __jerry.get ($2);
      try
      {
        obj[name] = to_set;
      }
      catch (e)
      {
        return __jerry.setErrorByValue (e);
      }
      return __jerry.ref (true);
    }
    , obj_val, prop_name_val, value_to_set);
} /* jerry_set_property */

jerry_value_t jerry_set_property_by_index (const jerry_value_t obj_value,
                                           uint32_t index,
                                           const jerry_value_t value_to_set)
{
  jerry_value_t obj_val = jerry_get_arg_value (obj_value);
  if (jerry_value_has_error_flag (value_to_set) ||
      !jerry_value_is_object (obj_val))
  {
    return TYPE_ERROR_ARG;
  }
  return (jerry_value_t) EM_ASM_INT (
    {
      var obj = __jerry.get ($0);
      var to_set = __jerry.get ($2);
      try
      {
        obj[$1] = to_set;
      }
      catch (e)
      {
        return __jerry.setErrorByValue (e);
      }
      return __jerry.ref (true);
    }
    , obj_val, index, value_to_set);
} /* jerry_set_property_by_index */

void jerry_init_property_descriptor_fields (jerry_property_descriptor_t *prop_desc_p)
{
  *prop_desc_p = (jerry_property_descriptor_t)
  {
    .value = jerry_create_undefined (),
    .getter = jerry_create_undefined (),
    .setter = jerry_create_undefined (),
  };
} /* jerry_init_property_descriptor_fields */

jerry_value_t jerry_define_own_property (const jerry_value_t obj_value,
                                         const jerry_value_t prop_name_value,
                                         const jerry_property_descriptor_t *pdp)
{
  jerry_value_t obj_val = jerry_get_arg_value (obj_value);
  jerry_value_t prop_name_val = jerry_get_arg_value (prop_name_value);
  if (!jerry_value_is_object (obj_val) && !jerry_value_is_string (obj_val))
  {
    return TYPE_ERROR_ARG;
  }
  if ((pdp->is_writable_defined || pdp->is_value_defined)
      && (pdp->is_get_defined || pdp->is_set_defined))
  {
    return TYPE_ERROR_ARG;
  }
  if (pdp->is_get_defined && !jerry_value_is_function (pdp->getter))
  {
    return TYPE_ERROR_ARG;
  }
  if (pdp->is_set_defined && !jerry_value_is_function (pdp->setter))
  {
    return TYPE_ERROR_ARG;
  }

  return (jerry_value_t) (EM_ASM_INT (
    {
      var obj = __jerry.get ($12 /* obj_val */);
      var name = __jerry.get ($13 /* prop_name_val */);
      var desc = {};
      if ($0 /* is_value_defined */)
      {
        desc.value = __jerry.get ($9);
      }
      if ($1 /* is_get_defined */)
      {
        desc.get = __jerry.get ($10);
      }
      if ($2 /* is_set_defined */)
      {
        desc.set = __jerry.get ($11);
      }
      if ($3 /* is_writable_defined */)
      {
        desc.writable = Boolean ($4 /* is_writable */);
      }
      if ($5 /* is_enumerable_defined */)
      {
        desc.enumerable = Boolean ($6 /* is_enumerable */);
      }
      if ($7 /* is_configurable */)
      {
        desc.configurable = Boolean ($8 /* is_configurable */);
      }

      Object.defineProperty (obj, name, desc);
      return __jerry.ref (Boolean (true));
    }
    , pdp->is_value_defined,    /* $0 */
    pdp->is_get_defined,      /* $1 */
    pdp->is_set_defined,      /* $2 */
    pdp->is_writable_defined, /* $3 */
    pdp->is_writable,         /* $4 */
    pdp->is_enumerable_defined,   /* $5 */
    pdp->is_enumerable,           /* $6 */
    pdp->is_configurable_defined, /* $7 */
    pdp->is_configurable,         /* $8 */
    pdp->value,   /* $9  */
    pdp->getter,  /* $10 */
    pdp->setter,  /* $11 */
    obj_val,      /* $12 */
    prop_name_val /* $13 */
  ));
} /* jerry_define_own_property */

bool
jerry_get_own_property_descriptor (const jerry_value_t obj_value, /**< object value */
                                   const jerry_value_t prop_name_value, /**< property name (string value) */
                                   jerry_property_descriptor_t *prop_desc_p) /**< property descriptor */
{
  jerry_value_t obj_val = jerry_get_arg_value (obj_value);
  jerry_value_t prop_name_val = jerry_get_arg_value (prop_name_value);
  if (!jerry_value_is_object (obj_val)
      || !jerry_value_is_string (prop_name_val))
  {
    return false;
  }
  /* Emscripten's setValue () only works with aligned accesses. The bool fields
   * of jerry_property_descriptor_t are not word aligned, so use word-sized temporary variables: */
  int32_t is_configurable_defined;
  int32_t is_configurable;
  int32_t is_enumerable_defined;
  int32_t is_enumerable;
  int32_t is_writable_defined;
  int32_t is_writable;
  int32_t is_value_defined;
  jerry_value_t value;
  int32_t is_set_defined;
  jerry_value_t setter;
  int32_t is_get_defined;
  jerry_value_t getter;

  const bool success = (bool) EM_ASM_INT (
    {
      try
      {
        var obj = __jerry.get ($0);
        var propName = __jerry.get ($1);
        var propDesc = Object.getOwnPropertyDescriptor (obj, propName);
        var assignFieldPair = function (fieldName, isDefinedTarget, valueTarget)
        {
          var isDefined = propDesc.hasOwnProperty (fieldName);
          setValue (isDefinedTarget, isDefined, 'i32*');
          if (isDefined)
          {
            var value = propDesc[fieldName];
            switch (fieldName)
            {
              case 'value':
              {
                value = __jerry.ref (value);
                break;
              }
              case 'set':
              case 'get':
              {
                value = __jerry.ref (value ? value : null);
                break;
              }
            }
            setValue (valueTarget, value, 'i32*');
          }
          else
          {
            setValue (valueTarget, __jerry.ref (undefined), 'i32*');
          }
        };
        assignFieldPair ('configurable', $2, $3);
        assignFieldPair ('enumerable', $4, $5);
        assignFieldPair ('writable', $6, $7);
        assignFieldPair ('value', $8, $9);
        assignFieldPair ('set', $10, $11);
        assignFieldPair ('get', $12, $13);
      }
      catch (e)
      {
        return false;
      }
    return true;
    }
    , /* $0: */ obj_val, /* $1: */ prop_name_val,
    /* $2: */ &is_configurable_defined, /* $3: */ &is_configurable,
    /* $4: */ &is_enumerable_defined, /* $5: */ &is_enumerable,
    /* $6: */ &is_writable_defined, /* $7: */ &is_writable,
    /* $8: */ &is_value_defined, /* $9: */ &value,
    /* $10: */ &is_set_defined, /* $11: */ &setter,
    /* $12: */ &is_get_defined, /* $13: */ &getter);

  if (success)
  {
    *prop_desc_p = (jerry_property_descriptor_t)
    {
      .is_configurable_defined = (bool) is_configurable_defined,
      .is_configurable = (bool) is_configurable,
      .is_enumerable_defined = (bool) is_enumerable_defined,
      .is_enumerable = (bool) is_enumerable,
      .is_value_defined = (bool) is_value_defined,
      .value = value,
      .is_set_defined = (bool) is_set_defined,
      .setter = setter,
      .is_get_defined = (bool) is_get_defined,
      .getter = getter,
    };
  }
  return success;
} /* jerry_get_own_property_descriptor */

void
jerry_free_property_descriptor_fields (const jerry_property_descriptor_t *prop_desc_p) /**< property descriptor */
{
  if (prop_desc_p->is_value_defined)
  {
    jerry_release_value (prop_desc_p->value);
  }

  if (prop_desc_p->is_get_defined)
  {
    jerry_release_value (prop_desc_p->getter);
  }

  if (prop_desc_p->is_set_defined)
  {
    jerry_release_value (prop_desc_p->setter);
  }
} /* jerry_free_property_descriptor_fields */


static jerry_value_t __attribute__((used))
_jerry_call_external_handler (jerry_external_handler_t func_obj_p,
                              const jerry_value_t func_obj_val,
                              const jerry_value_t this_val,
                              const jerry_value_t args_p[],
                              jerry_size_t args_count)
{
  return (func_obj_p) (func_obj_val, this_val, args_p, args_count);
} /* _jerry_call_external_handler */

jerry_value_t jerry_call_function (const jerry_value_t func_obj_val,
                                   const jerry_value_t this_val,
                                   const jerry_value_t args_p[],
                                   jerry_size_t args_count)
{
  if (!jerry_value_is_function (func_obj_val))
  {
    return TYPE_ERROR_ARG;
  }

  return (jerry_value_t) EM_ASM_INT (
    {
      var func_obj = __jerry.get ($0);
      var this_val = __jerry.get ($1);
      var args =[];
      for (var i = 0; i < $3; ++i)
      {
        args.push (__jerry.get (getValue ($2 + i * 4, 'i32')));
      }
      try
      {
        var rv = func_obj.apply (this_val, args);
      }
      catch (e)
      {
        return __jerry.setErrorByValue (e);
      }
      return __jerry.ref (rv);
    }
    , func_obj_val, this_val, args_p, args_count);
} /* jerry_call_function */

jerry_value_t jerry_construct_object (const jerry_value_t func_obj_val,
                                      const jerry_value_t args_p[],
                                      jerry_size_t args_count)
{
  if (!jerry_value_is_constructor (func_obj_val))
  {
    return TYPE_ERROR_ARG;
  }
  return (jerry_value_t) EM_ASM_INT (
    {
      var constructor = __jerry.get ($0);
      var args =[];
      for (var i = 0; i < $2; ++i)
      {
        args.push (__jerry.get (getValue ($1 + (i * 4 /* sizeof (i32) */), 'i32')));
      }
      /* Call the constructor with new object as `this`. */
      var bindArgs =[null].concat (args);
      var boundConstructor = constructor.bind.apply (constructor, bindArgs);
      try
      {
        var rv = new boundConstructor ();
      }
      catch (e)
      {
        return __jerry.setErrorByValue (e);
      }
      return __jerry.ref (rv);
    }
    , func_obj_val, args_p, args_count);
} /* jerry_construct_object */

jerry_length_t
jerry_get_string_length (const jerry_value_t val) /**< input string */
{
  jerry_value_t value = jerry_get_arg_value (val);
  if (!jerry_value_is_string (value))
  {
    return 0;
  }

  return (jerry_length_t) EM_ASM_INT (
    {
      var str = __jerry.get ($0);
      return str.length;
    }
    , value);
} /* jerry_get_string_length */

jerry_size_t jerry_string_to_char_buffer (const jerry_value_t val,
                                          jerry_char_t *buffer_p,
                                          jerry_size_t buffer_size)
{
  jerry_value_t value = jerry_get_arg_value (val);
  const jerry_size_t str_size = jerry_get_string_size (value);
  if (str_size == 0 || buffer_size < str_size || buffer_p == NULL)
  {
    return 0;
  }
  return (jerry_size_t) EM_ASM_INT (
    {
      var str = __jerry.get ($0);
      return Module.stringToCESU8DataOnly (str, $1, $2);
    }
    , value, buffer_p, buffer_size);
} /* jerry_string_to_char_buffer */

jerry_value_t jerry_get_object_keys (const jerry_value_t val)
{
  jerry_value_t value = jerry_get_arg_value (val);
  if (!jerry_value_is_object (value))
  {
    return TYPE_ERROR_ARG;
  }
  return (jerry_value_t) EM_ASM_INT (
    {
      return __jerry.ref (Object.keys (__jerry.get ($0)));
    }
    , value);
} /* jerry_get_object_keys */

jerry_value_t jerry_get_prototype (const jerry_value_t val)
{
  jerry_value_t value = jerry_get_arg_value (val);
  if (!jerry_value_is_object (value))
  {
    return TYPE_ERROR_ARG;
  }
  return (jerry_value_t) EM_ASM_INT (
    {
      if (!__jerry.hasProto)
      {
        throw new Error ('Not implemented, host engine does not implement __proto__.');
      }
      return __jerry.ref (__jerry.get ($0).__proto__);
    }
    , value);
} /* jerry_get_prototype */

jerry_value_t jerry_set_prototype (const jerry_value_t obj_value,
                                   const jerry_value_t proto_obj_val)
{
  jerry_value_t obj_val = jerry_get_arg_value (obj_value);
  EM_ASM_ARGS (
    {
      if (!__jerry.hasProto)
      {
        throw new Error ('Not implemented, host engine does not implement __proto__.');
      }
      var obj = __jerry.get ($0);
      var proto = __jerry.get ($1);
      obj.__proto__ = proto;
    }
    , obj_val, proto_obj_val);
  return jerry_create_boolean (true);
} /* jerry_set_prototype */

bool jerry_get_object_native_handle (const jerry_value_t obj_value,
                                     uintptr_t *out_handle_p)
{
  jerry_value_t obj_val = jerry_get_arg_value (obj_value);
  if (!jerry_value_is_object (obj_val))
  {
    return false;
  }
  return (bool) EM_ASM_INT (
    {
      var value = __jerry.get ($0);
      var internalProps = __jerry._jerryInternalPropsWeakMap.get (value);
      var handle = internalProps.nativeHandle;
      if (handle === undefined)
      {
        return false;
      }
      if ($1)
      {
        Module.setValue ($1, handle, '*');
      }
      return true;
    }
    , obj_val, out_handle_p);
} /* jerry_get_object_native_handle */

void jerry_set_object_native_handle (const jerry_value_t obj_value,
                                     uintptr_t handle_p,
                                     jerry_object_free_callback_t freecb_p)
{
  jerry_value_t obj_val = jerry_get_arg_value (obj_value);
  if (jerry_value_is_object (obj_val))
  {
    EM_ASM_INT (
      {
        var value = __jerry.get ($0);
        var internalProps = __jerry._jerryInternalPropsWeakMap.get (value);
        internalProps.nativeHandle = $1;
        internalProps.nativeHandleFreeCb = $2;
      }
      , obj_val, handle_p, freecb_p);
  }
} /* jerry_set_object_native_handle */

void
jerry_set_object_native_pointer (const jerry_value_t obj_value, /**< object to set native pointer in */
                                 void *native_pointer_p, /**< native pointer */
                                 const jerry_object_native_info_t *native_info_p) /**< object's native type info */
{
  jerry_value_t obj_val = jerry_get_arg_value (obj_value);
  if (jerry_value_is_object (obj_val))
  {
    EM_ASM_INT (
      {
        var value = __jerry.get ($0);
        var internalProps = __jerry._jerryInternalPropsWeakMap.get (value);
        internalProps.nativePtr = $1;
        internalProps.nativeInfo = $2;
      }
      , obj_val, native_pointer_p, native_info_p);
  }
} /* jerry_set_object_native_pointer */

bool
jerry_get_object_native_pointer (const jerry_value_t obj_value, /**< object to get native pointer from */
                                 void **out_native_pointer_p, /**< [out] native pointer */
                                 const jerry_object_native_info_t **out_native_info_p) /**< [out] the type info
                                                                                        *   of the native pointer */
{
  jerry_value_t obj_val = jerry_get_arg_value (obj_value);
  if (!jerry_value_is_object (obj_val))
  {
    return false;
  }
  return (bool) EM_ASM_INT (
    {
      var value = __jerry.get ($0);
      var internalProps = __jerry._jerryInternalPropsWeakMap.get (value);
      var ptr = internalProps.nativePtr;
      if (ptr === undefined)
      {
        return false;
      }
      if ($1)
      {
        Module.setValue ($1, ptr, '*');
      }
      if ($2)
      {
        Module.setValue ($2, internalProps.nativeInfo, '*');
      }
      return true;
    }
    , obj_val, out_native_pointer_p, out_native_info_p);
} /* jerry_get_object_native_pointer */

static bool __attribute__ ((used))
_jerry_call_foreach_cb (jerry_object_property_foreach_t foreach_p,
                        const jerry_value_t property_name,
                        const jerry_value_t property_value,
                        void *user_data_p)
{
  return foreach_p (property_name, property_value, user_data_p);
} /* _jerry_call_foreach_cb */

bool
jerry_foreach_object_property (const jerry_value_t obj_value, /**< object value */
                               jerry_object_property_foreach_t foreach_p, /**< foreach function */
                               void *user_data_p) /**< user data for foreach function */
{
  jerry_value_t obj_val = jerry_get_arg_value (obj_value);
  return (bool) EM_ASM_INT (
    {
      var obj = __jerry.get ($0);
      try
      {
        for (var propName in obj)
        {
          var propNameRef = __jerry.ref (propName);
          var propValRef = __jerry.ref (obj[propName]);
          var shouldContinue = Module.ccall (
            '_jerry_call_foreach_cb',
            'number',
            ['number', 'number'],
            [$1, propNameRef, propValRef, $2]);
          if (!shouldContinue)
          {
            return true;
          }
        }
      }
      catch (e)
      {
        return false;
      };
      return true;
    }
    , obj_val, foreach_p, user_data_p
  );
} /* jerry_foreach_object_property */

jerry_value_t
jerry_resolve_or_reject_promise (jerry_value_t promise,
                                 jerry_value_t argument,
                                 bool is_resolve)
{
  promise = jerry_get_arg_value (promise);
  argument = jerry_get_arg_value (argument);
  if (!jerry_value_is_promise (promise))
  {
    return TYPE_ERROR_ARG;
  }

  return (jerry_value_t) EM_ASM_INT (
    {
      var p = __jerry.get ($0);
      var arg = __jerry.get ($1);
      var internalProps = __jerry._jerryInternalPropsWeakMap.get (p);
      var func;
      if ($2)
      {
        func = internalProps.promiseResolveFunc;
      }
      else
      {
        func = internalProps.promiseRejectFunc;
      }

      try
      {
        var rv = func (arg);
      }
      catch (e)
      {
        return __jerry.setErrorByValue (e);
      }
      return __jerry.ref (rv);
    }
    , promise, argument, is_resolve
  );
} /* jerry_resolve_or_reject_promise */

static void __attribute__ ((used))
_jerry_call_native_object_free_callbacks (const jerry_object_native_info_t *native_info_p,
                                          void *native_pointer_p,
                                          jerry_object_free_callback_t native_handle_freecb_p,
                                          uintptr_t native_handle)
{
  if (native_info_p && native_info_p->free_cb)
  {
    native_info_p->free_cb (native_pointer_p);
  }
  if (native_handle_freecb_p)
  {
    native_handle_freecb_p (native_handle);
  }
} /* _jerry_call_native_object_free_callbacks */

/****************************************
 * Error flag manipulation functions
 *
 * The error flag is stored alongside the value in __jerry.
 * This allows for us to keep a valid value, like jerryscript does, and be able
 * to add / remove a flag specifying whether there was an error or not.
 ***************************************/

bool jerry_value_has_error_flag (const jerry_value_t value)
{
  return (bool) (EM_ASM_INT (
    {
      return __jerry.isError ($0);
    }
    , value));
} /* jerry_value_has_error_flag */

bool
jerry_value_has_abort_flag (const jerry_value_t value) /**< api value */
{
  EM_ASM (
      {
          throw new Error ("jerry_value_has_abort_flag () is not implemented");
      }
  );
  JERRY_UNUSED (value);
  return false;
} /* jerry_value_has_abort_flag */

void jerry_value_clear_error_flag (jerry_value_t *value_p)
{
  *value_p = (jerry_value_t) EM_ASM_INT (
    {
      return __jerry.setError ($0, false);
    }
    , *value_p);
} /* jerry_value_clear_error_flag */

void jerry_value_set_error_flag (jerry_value_t *value_p)
{
  *value_p = (jerry_value_t) EM_ASM_INT (
    {
      return __jerry.setError ($0, true);
    }
    , *value_p);
} /* jerry_value_set_error_flag */

void
jerry_value_set_abort_flag (jerry_value_t *value_p)
{
  EM_ASM (
      {
          throw new Error ("jerry_value_set_abort_flag () is not implemented");
      }
  );
  JERRY_UNUSED (value_p);
} /* jerry_value_set_abort_flag */

jerry_value_t jerry_get_value_without_error_flag (jerry_value_t value)
{
  return (jerry_value_t) (EM_ASM_INT (
    {
      return __jerry.getRefFromError ($0);
    }
    , value));
} /* jerry_get_value_without_error_flag */

jerry_error_t
jerry_get_error_type (const jerry_value_t value) /**< api value */
{
  jerry_value_t object = jerry_get_arg_value (value);
  return (jerry_error_t) EM_ASM_INT (
    {
      var value = __jerry.get ($0);
      if (value === null || value === undefined)
      {
        return 0;
      }
      if (!__jerry.hasProto)
      {
        throw new Error ('Not implemented, host engine does not implement __proto__.');
      }

      switch (value.__proto__)
      {
        case Error.prototype:
          return 1;
        case EvalError.prototype:
          return 2;
        case RangeError.prototype:
          return 3;
        case ReferenceError.prototype:
          return 4;
        case SyntaxError.prototype:
          return 5;
        case TypeError.prototype:
          return 6;
        case URIError.prototype;
          return 7;
        default:
          return 0;
      }
    }
    , object);
} /* jerry_get_error_type */

/*************************************
 * Converters of `jerry_value_t`
 ***************************************/

bool jerry_value_to_boolean (const jerry_value_t value)
{
  if (jerry_value_has_error_flag (value))
  {
    return false;
  }
  return (bool) EM_ASM_INT (
    {
      return Boolean (__jerry.get ($0));
    }
    , value);
} /* jerry_value_to_boolean */



jerry_value_t jerry_value_to_number (const jerry_value_t value)
{
  if (jerry_value_has_error_flag (value))
  {
    return TYPE_ERROR_FLAG;
  }
  return (jerry_value_t) EM_ASM_INT (
    {
      return __jerry.ref (Number (__jerry.get ($0)));
    }
    , value);
} /* jerry_value_to_number */

jerry_value_t jerry_value_to_object (const jerry_value_t value)
{
  if (jerry_value_has_error_flag (value))
  {
    return TYPE_ERROR_FLAG;
  }
  return (jerry_value_t) EM_ASM_INT (
    {
      return __jerry.ref (new Object (__jerry.get ($0)));
    }
    , value);
} /* jerry_value_to_object */

jerry_value_t jerry_value_to_primitive (const jerry_value_t value)
{
  if (jerry_value_has_error_flag (value))
  {
    return TYPE_ERROR_FLAG;
  }
  return (jerry_value_t) EM_ASM_INT (
    {
      var val = __jerry.get ($0);
      var rv;
      if ((typeof val === 'object' && val != null)
      || (typeof val === 'function'))
      {
        rv = val.valueOf (); // unbox
      }
      else
      {
        rv = val; // already a primitive
      }
      return __jerry.ref (rv);
    }
    , value);
} /* jerry_value_to_primitive */

jerry_value_t jerry_value_to_string (const jerry_value_t value)
{
  if (jerry_value_has_error_flag (value))
  {
    return TYPE_ERROR_FLAG;
  }
  return (jerry_value_t) EM_ASM_INT (
    {
      return __jerry.ref (String (__jerry.get ($0)));
    }
    , value);
} /* jerry_value_to_string */

void jerry_get_memory_limits (size_t *out_data_bss_brk_limit_p,
                              size_t *out_stack_limit_p)
{
  *out_data_bss_brk_limit_p = 0;
  *out_stack_limit_p = 0;
} /* jerry_get_memory_limits */

/**
 * Get heap memory stats.
 *
 * @return true - get the heap stats successful
 *         false - otherwise. Usually it is because the MEM_STATS feature is not enabled.
 */
bool
jerry_get_memory_stats (jerry_heap_stats_t *out_stats_p) /**< [out] heap memory stats */
{
  (void) out_stats_p;
  return false;
} /* jerry_get_memory_stats */

/**********************************
 * Jerry Init and Cleanup Related Functions
 ******************************/

void jerry_init (jerry_init_flag_t flags)
{
  EM_ASM (__jerry.reset ());
  (void) flags;
} /* jerry_init */

void jerry_cleanup (void)
{
  for (jerry_context_data_header_t *this_p = JERRY_CONTEXT (context_data_p), *next_p = NULL;
       this_p != NULL;
       this_p = next_p)
  {
    next_p = this_p->next_p;
    this_p->manager_p->deinit_cb (JERRY_CONTEXT_DATA_HEADER_USER_DATA (this_p));
    free (this_p);
  }

  jerry_gc ();
} /* jerry_cleanup */

void *
jerry_get_context_data (const jerry_context_data_manager_t *manager_p)
{
  jerry_context_data_header_t *item_p;

  for (item_p = JERRY_CONTEXT (context_data_p); item_p != NULL; item_p = item_p->next_p)
  {
    if (item_p->manager_p == manager_p)
    {
      return JERRY_CONTEXT_DATA_HEADER_USER_DATA (item_p);
    }
  }

  item_p = malloc (sizeof (jerry_context_data_header_t) + manager_p->bytes_needed);
  item_p->manager_p = manager_p;
  item_p->next_p = JERRY_CONTEXT (context_data_p);
  JERRY_CONTEXT (context_data_p) = item_p;
  void *ret = JERRY_CONTEXT_DATA_HEADER_USER_DATA (item_p);

  memset (ret, 0, manager_p->bytes_needed);
  if (manager_p->init_cb)
  {
    manager_p->init_cb (ret);
  }

  return ret;
} /* jerry_get_context_data */

bool jerry_is_feature_enabled (const jerry_feature_t feature)
{
  switch (feature)
  {
    case JERRY_FEATURE_ERROR_MESSAGES:
    case JERRY_FEATURE_JS_PARSER:
    {
      return true;
    }
    case JERRY_FEATURE_CPOINTER_32_BIT:
    case JERRY_FEATURE_DEBUGGER:
    case JERRY_FEATURE_MEM_STATS:
    case JERRY_FEATURE_PARSER_DUMP:
    case JERRY_FEATURE_REGEXP_DUMP:
    case JERRY_FEATURE_SNAPSHOT_EXEC:
    case JERRY_FEATURE_SNAPSHOT_SAVE:
    case JERRY_FEATURE_VM_EXEC_STOP:
    case JERRY_FEATURE__COUNT:
    {
      return false;
    }
  }
} /* jerry_is_feature_enabled */

void
jerry_set_vm_exec_stop_callback (jerry_vm_exec_stop_callback_t stop_cb, /**< periodically called user function */
                                 void *user_p, /**< pointer passed to the function */
                                 uint32_t frequency) /**< frequency of the function call */
{
  EM_ASM (
    {
      console.warn ("jerry_set_vm_exec_stop_callback () is not implemented, ignoring the call.");
    }
  );
  JERRY_UNUSED (stop_cb);
  JERRY_UNUSED (user_p);
  JERRY_UNUSED (frequency);
} /* jerry_set_vm_exec_stop_callback */

bool
jerry_value_is_arraybuffer (const jerry_value_t value) /**< value to check if it is an ArrayBuffer */
{
  jerry_value_t buffer = jerry_get_arg_value (value);
  return (bool) EM_ASM_INT (
    {
      var val = __jerry.get ($0);
      return val instanceof ArrayBuffer;
    }
    ,
    buffer);
} /* jerry_value_is_arraybuffer */

jerry_value_t
jerry_create_arraybuffer (const jerry_length_t size) /**< size of the ArrayBuffer to create */
{
  return (jerry_value_t) EM_ASM_INT (
    {
      var val = new ArrayBuffer ($0);
      return __jerry.ref (val);
    }
    ,
    size);
} /* jerry_create_arraybuffer */

jerry_value_t
jerry_create_arraybuffer_external (const jerry_length_t size, /**< size of the buffer to used */
                                   uint8_t *buffer_p, /**< buffer to use as the ArrayBuffer's backing */
                                   jerry_object_native_free_callback_t free_cb) /**< buffer free callback */
{
  EM_ASM (
    {
      throw new Error ("jerry_create_arraybuffer_external () is not implemented!");
    }
  );
  JERRY_UNUSED (size);
  JERRY_UNUSED (buffer_p);
  JERRY_UNUSED (free_cb);
  return jerry_create_undefined ();
} /* jerry_create_arraybuffer_external */

/**
 * Copy bytes into the ArrayBuffer from a buffer.
 *
 * Note:
 *     * if the object passed is not an ArrayBuffer will return 0.
 *
 * @return number of bytes copied into the ArrayBuffer.
 */
jerry_length_t
jerry_arraybuffer_write (const jerry_value_t value, /**< target ArrayBuffer */
                         jerry_length_t offset, /**< start offset of the ArrayBuffer */
                         const uint8_t *buf_p, /**< buffer to copy from */
                         jerry_length_t buf_size) /**< number of bytes to copy from the buffer */
{
  jerry_value_t buffer = jerry_get_arg_value (value);

  return (jerry_length_t) EM_ASM_INT (
    {
      var buffer = __jerry.get ($0);
      var offset = $1;
      var bufPtr = $2;
      var bufSize = $3;
      if (false === buffer instanceof ArrayBuffer)
      {
        return 0;
      }
      var length = buffer.byteLength;
      if (offset >= length)
      {
        return 0;
      }
      var copyCount = Math.min (length - offset, bufSize);
      if (copyCount > 0)
      {
        var dest = new Uint8Array (buffer);
        var src = Module.HEAPU8.subarray (bufPtr, bufPtr + copyCount);
        dest.set (src, offset);
      }
      return copyCount;
    }
    ,
    buffer, offset, buf_p, buf_size);
} /* jerry_arraybuffer_write */

/**
 * Copy bytes from a buffer into an ArrayBuffer.
 *
 * Note:
 *     * if the object passed is not an ArrayBuffer will return 0.
 *
 * @return number of bytes read from the ArrayBuffer.
 */
jerry_length_t
jerry_arraybuffer_read (const jerry_value_t value, /**< ArrayBuffer to read from */
                        jerry_length_t offset, /**< start offset of the ArrayBuffer */
                        uint8_t *buf_p, /**< destination buffer to copy to */
                        jerry_length_t buf_size) /**< number of bytes to copy into the buffer */
{
  jerry_value_t buffer = jerry_get_arg_value (value);

  return (jerry_length_t) EM_ASM_INT (
    {
      var buffer = __jerry.get ($0);
      var offset = $1;
      var bufPtr = $2;
      var bufSize = $3;
      if (false === buffer instanceof ArrayBuffer)
      {
        return 0;
      }
      var length = buffer.byteLength;
      if (offset >= length)
      {
        return 0;
      }
      var copyCount = Math.min (length - offset, bufSize);
      if (copyCount > 0)
      {
        var src = new Uint8Array (buffer, offset, copyCount);
        Module.HEAPU8.set (src, bufPtr);
      }
      return copyCount;
    }
    ,
    buffer, offset, buf_p, buf_size);
} /* jerry_arraybuffer_read */

jerry_length_t
jerry_get_arraybuffer_byte_length (const jerry_value_t value) /**< ArrayBuffer */
{
  jerry_value_t buffer = jerry_get_arg_value (value);

  return (jerry_length_t) EM_ASM_INT (
    {
      var buffer = __jerry.get ($0);
      if (false === buffer instanceof ArrayBuffer)
      {
        return 0;
      }
      return buffer.byteLength;
    }
    ,
    buffer);
} /* jerry_get_arraybuffer_byte_length */

uint8_t *
jerry_get_arraybuffer_pointer (const jerry_value_t value) /**< Array Buffer to use */
{
  EM_ASM (
    {
      throw new Error ("jerry_get_arraybuffer_pointer () is not implemented!");
    }
  );
  JERRY_UNUSED (value);
  return NULL;
} /* jerry_get_arraybuffer_pointer */

bool
jerry_value_is_typedarray (jerry_value_t value) /**< value to check if it is a TypedArray */
{
  jerry_value_t array = jerry_get_arg_value (value);

  return (bool) EM_ASM_INT (
    {
      var array = __jerry.get ($0);
      return (array instanceof Object.getPrototypeOf (Uint8Array));
    }
    ,
    array);
} /* jerry_value_is_typedarray */

// Disgusting hack
#define ONLY_LENGTH_VALUE (0)

jerry_value_t
jerry_create_typedarray (jerry_typedarray_type_t type_name, /**< type of TypedArray to create */
                         jerry_length_t length) /**< element count of the new TypedArray */
{
  return jerry_create_typedarray_for_arraybuffer_sz (type_name, ONLY_LENGTH_VALUE, 0, length);
} /* jerry_create_typedarray */

jerry_value_t
jerry_create_typedarray_for_arraybuffer_sz (jerry_typedarray_type_t type_name, /**< type of TypedArray to create */
                                            const jerry_value_t arraybuffer, /**< ArrayBuffer to use */
                                            jerry_length_t byte_offset, /**< offset for the ArrayBuffer */
                                            jerry_length_t length) /**< number of elements to use from ArrayBuffer */
{
  const bool only_length = (arraybuffer == ONLY_LENGTH_VALUE);
  jerry_value_t buffer = only_length ? jerry_create_undefined () : jerry_get_arg_value (arraybuffer);

  return (jerry_value_t) EM_ASM_INT (
    {
      var Ctor = __jerry.typedArrayConstructorByTypeNameMap[$0];
      if (!Ctor)
      {
        return __jerry.setErrorByValue (new TypeError ("incorrect type for TypedArray."));
      }
      var onlyLength = $4;
      var buffer = __jerry.get ($1);
      var byteOffset = $2;
      var length = $3;
      if (onlyLength)
      {
        return __jerry.ref (new Ctor (length));
      }
      else if (!(buffer instanceof ArrayBuffer))
      {
        return __jerry.setErrorByValue (new TypeError ("Argument is not an ArrayBuffer"));
      }
      return __jerry.ref (new Ctor (buffer, byteOffset, length));
    }
    ,
    type_name, buffer, byte_offset, length, only_length);
} /* jerry_create_typedarray_for_arraybuffer_sz */

jerry_value_t
jerry_create_typedarray_for_arraybuffer (jerry_typedarray_type_t type_name, /**< type of TypedArray to create */
                                         const jerry_value_t arraybuffer) /**< ArrayBuffer to use */
{
  jerry_length_t byteLength = jerry_get_arraybuffer_byte_length (arraybuffer);
  return jerry_create_typedarray_for_arraybuffer_sz (type_name, arraybuffer, 0, byteLength);
} /* jerry_create_typedarray_for_arraybuffer */

jerry_typedarray_type_t
jerry_get_typedarray_type (jerry_value_t value) /**< object to get the TypedArray type */
{
  jerry_value_t array = jerry_get_arg_value (value);

  return (jerry_value_t) EM_ASM_INT (
    {
      var array = __jerry.get ($0);
      var constructorByTypeNameMap = __jerry.typedArrayConstructorByTypeNameMap;
      for (var typeName in constructorByTypeNameMap)
      {
        if (array instanceof constructorByTypeNameMap[typeName])
        {
          return typeName;
        }
      }
      return 0;
    }
    ,
    array);
} /* jerry_get_typedarray_type */

jerry_length_t
jerry_get_typedarray_length (jerry_value_t value) /**< TypedArray to query */
{
  jerry_value_t array = jerry_get_arg_value (value);

  return (jerry_length_t) EM_ASM_INT (
    {
      var array = __jerry.get ($0);
      return (array instanceof Object.getPrototypeOf (Uint8Array)) ? array.length : 0;
    }
    ,
    array);
} /* jerry_get_typedarray_length */

jerry_value_t
jerry_get_typedarray_buffer (jerry_value_t value, /**< TypedArray to get the arraybuffer from */
                             jerry_length_t *byte_offset, /**< [out] byteOffset property */
                             jerry_length_t *byte_length) /**< [out] byteLength property */
{
  jerry_value_t array = jerry_get_arg_value (value);

  return (jerry_length_t) EM_ASM_INT (
    {
      var array = __jerry.get ($0);
      var byteOffsetPtr = $1;
      var byteLengthPtr = $2;
      if (!(array instanceof Object.getPrototypeOf (Uint8Array)))
      {
        return __jerry.setErrorByValue (new TypeError ("Object is not a TypedArray."));
      }
      var buffer = array.buffer;
      if (0 !== byteOffsetPtr)
      {
        Module.setValue (byteOffsetPtr, array.byteOffset, 'i32');
      }
      if (0 !== byteLengthPtr)
      {
        Module.setValue (byteLengthPtr, array.byteLength, 'i32');
      }
      return __jerry.ref (buffer);
    }
    ,
    array, byte_offset, byte_length);
} /* jerry_get_typedarray_buffer */
