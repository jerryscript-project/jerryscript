# jerry_run_simple

**Summary**

The simplest way to run JavaScript.

**Prototype**

```c
jerry_completion_code_t
jerry_run_simple (const jerry_char_t *script_source,
                  size_t script_source_size,
                  jerry_flag_t flags);
```

- `script_source` - source code, it must be a valid utf8 string.
- `script_source_size` - size of source code buffer, in bytes.
- return value - completion code, indicating whether execution was successful
                 (`JERRY_COMPLETION_CODE_OK`), or an unhandled JavaScript exception occurred
                 (`JERRY_COMPLETION_CODE_UNHANDLED_EXCEPTION`).

**Example**

```c
{
  const jerry_char_t *script = "print ('Hello, World!');";

  jerry_run_simple (script, strlen ((const char *) script), JERRY_FLAG_EMPTY);
}
```

**See also**

- [jerry_init](#jerry_init)
- [jerry_cleanup](#jerry_cleanup)
- [jerry_parse](#jerry_parse)
- [jerry_run](#jerry_run)


# jerry_init

**Summary**

Initializes the JerryScript engine, making possible to run JavaScript code and perform operations on
JavaScript values.

**Prototype**

```c
void
jerry_init (jerry_flag_t flags);
```

`flags` - combination of various engine configuration flags:

- `JERRY_FLAG_EMPTY` - no flags, just initialize in default configuration.
- `JERRY_FLAG_SHOW_OPCODES` - print compiled byte-code.
- `JERRY_FLAG_MEM_STATS` - dump memory statistics.
- `JERRY_FLAG_MEM_STATS_SEPARATE` - dump memory statistics and reset peak values after parse.
- `JERRY_FLAG_ENABLE_LOG` - enable logging.

**Example**

```c
{
  jerry_init (JERRY_FLAG_SHOW_OPCODES | JERRY_FLAG_ENABLE_LOG);

  // ...

  jerry_cleanup ();
}
```

**See also**

- [jerry_cleanup](#jerry_cleanup)

# jerry_cleanup

**Summary**

Finish JavaScript engine execution, freeing memory and JavaScript values.

*Note*: JavaScript values, received from engine, will be inaccessible after the cleanup.

**Prototype**

```c
void
jerry_cleanup (void);
```

**See also**

- [jerry_init](#jerry_init)


# jerry_parse

**Summary**

Parse specified script to execute in Global scope.

*Note*: Current API doesn't permit replacement or modification of Global scope's code without engine
restart, which means 'jerry_parse' can be invoked only once between `jerry_init` and `jerry_cleanup`.

**Prototype**

```c
bool
jerry_parse (const jerry_char_t *source_p,
             size_t source_size,
             jerry_object_t **error_obj_p);
```

- `source_p` - string, containing source code to parse. It must be a valid utf8 string.
- `source_size` - size of the string, in bytes.
- `error_obj_p` - error object (output parameter).

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);

  char script [] = "print ('Hello, World!');";
  size_t script_size = strlen ((const char *) script);

  jerry_object_t *error_object_p = NULL;
  if (!jerry_parse (script, script_size, &error_object_p))
  {
    /* Error object must be freed, if parsing failed */
    jerry_release_object (error_object_p);
  }

  jerry_cleanup ();
}
```

**See also**

- [jerry_run](#jerry_run)


# jerry_parse_and_save_snapshot

**Summary**

Generate snapshot from the specified source code.

**Prototype**

```c
size_t
jerry_parse_and_save_snapshot (const jerry_char_t *source_p,
                               size_t source_size,
                               bool is_for_global,
                               uint8_t *buffer_p,
                               size_t buffer_size);
```

- `source_p` - script source, it must be a valid utf8 string.
- `source_size` - script source size, in bytes.
- `is_for_global` - snapshot would be executed as global (true) or eval (false).
- `buffer_p` - buffer to save snapshot to.
- `buffer_size` - the buffer's size.
- return value
  - the size of snapshot, if it was generated succesfully (i.e. there are no syntax errors in source
    code, buffer size is sufficient, and snapshot support is enabled in current configuration through
    JERRY_ENABLE_SNAPSHOT)
  - 0 otherwise.

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);

  static uint8_t global_mode_snapshot_buffer[1024];
  const char *code_to_snapshot_p = "(function () { return 'string from snapshot'; }) ();";

  size_t global_mode_snapshot_size = jerry_parse_and_save_snapshot ((jerry_char_t *) code_to_snapshot_p,
                                                                    strlen (code_to_snapshot_p),
                                                                    true,
                                                                    global_mode_snapshot_buffer,
                                                                    sizeof (global_mode_snapshot_buffer));
}
```

**See also**

- [jerry_init](#jerry_init)
- [jerry_exec_snapshot](#jerry_exec_snapshot)


# jerry_run

**Summary**

Run code of Global scope.

*Note*: The code should be previously registered through `jerry_parse`.

**Prototype**

```c
jerry_completion_code_t
jerry_run (jerry_value_t *error_value_p);
```

- `error_value_p` - error value (output parameter).
- return value - completion code that indicates whether run performed successfully.
  - `JERRY_COMPLETION_CODE_OK` - successful completion
  - `JERRY_COMPLETION_CODE_UNHANDLED_EXCEPTION` - an unhandled JavaScript exception occurred
  - `JERRY_COMPLETION_CODE_INVALID_SNAPSHOT_VERSION` - snapshot version mismatch
  - `JERRY_COMPLETION_CODE_INVALID_SNAPSHOT_FORMAT` - snapshot format is not valid

**Example**

```c
{
  const jerry_char_t script[] = "print ('Hello, World!');";
  size_t script_size = strlen ((const char *) script);

  /* Initialize engine */
  jerry_init (JERRY_FLAG_EMPTY);

  /* Setup Global scope code */
  jerry_object_t *error_object_p = NULL;
  if (!jerry_parse (script, script_size, &error_object_p))
  {
    /* Error object must be freed, if parsing failed */
    jerry_release_object (error_object_p);
  }
  else
  {
    /* Execute Global scope code
     *
     * Note:
     *      Initialization of 'error_value' is not mandatory here.
     */
    jerry_value_t error_value = jerry_create_undefined_value ();
    jerry_completion_code_t return_code = jerry_run (&error_value);

    if (return_code == JERRY_COMPLETION_CODE_UNHANDLED_EXCEPTION)
    {
      /* Error value must be freed, if 'jerry_run' returns with an unhandled exception */
      jerry_release_value (error_value);
    }
  }

  jerry_cleanup ();
}
```

**See also**

- [jerry_parse](#jerry_parse)


# jerry_exec_snapshot

**Summary**

Execute snapshot from the specified buffer.

**Prototype**

```c
jerry_completion_code_t
jerry_exec_snapshot (const void *snapshot_p,
                     size_t snapshot_size,
                     bool copy_bytecode,
                     jerry_value_t *retval_p);
```

- `snapshot_p` - pointer to snapshot
- `snapshot_size` - size of snapshot
- `copy_bytecode` - flag, indicating whether the passed snapshot buffer should be copied to the
   engine's memory. If set the engine should not reference the buffer after the function returns
   (in this case, the passed buffer could be freed after the call). Otherwise (if the flag is not
   set) - the buffer could only be freed after the engine stops (i.e. after call to jerry_cleanup).
- `retval_p` - return value, ECMA-262 'undefined' if code is executed as global scope code.
- return value - completion code that indicates whether run performed successfully. Same as before.

**Example**

```c
{
  bool is_ok;
  jerry_value_t res;
  static uint8_t global_mode_snapshot_buffer[1024];
  const char *code_to_snapshot_p = "(function () { return 'string from snapshot'; }) ();";

  jerry_init (JERRY_FLAG_EMPTY);
  size_t global_mode_snapshot_size = jerry_parse_and_save_snapshot ((jerry_char_t *) code_to_snapshot_p,
                                                                    strlen (code_to_snapshot_p),
                                                                    true,
                                                                    global_mode_snapshot_buffer,
                                                                    sizeof (global_mode_snapshot_buffer));
  jerry_cleanup ();

  jerry_init (JERRY_FLAG_EMPTY);

  is_ok = (jerry_exec_snapshot (global_mode_snapshot_buffer,
                                global_mode_snapshot_size,
                                false,
                                &res) == JERRY_COMPLETION_CODE_OK);
}
```

**See also**

- [jerry_init](#jerry_init)
- [jerry_cleanup](#jerry_cleanup)
- [jerry_parse_and_save_snapshot](#jerry_parse_and_save_snapshot)


# jerry_get_memory_limits

**Summary**

Gets configured memory limits of JerryScript.

**Prototype**

```c
void
jerry_get_memory_limits (size_t *out_data_bss_brk_limit_p,
                         size_t *out_stack_limit_p);
```

- `out_data_bss_brk_limit_p` - out parameter, that gives the maximum size of data + bss + brk sections.
- `out_stack_limit_p` - out parameter, that gives the maximum size of the stack.

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);

  size_t stack_limit;
  size_t data_dss_brk_limit;
  jerry_get_memory_limits (&stack_limit, &data_dss_brk_limit);
}
```

**See also**

- [jerry_init](#jerry_init)


# jerry_gc

**Summary**

Performs garbage collection.

**Prototype**

```c
void
jerry_gc (void);
```

**Example**

```c
jerry_gc ();
```


# jerry_eval

**Summary**

Perform JavaScript `eval`.

**Prototype**

```c
jerry_completion_code_t
jerry_eval (const jerry_char_t *source_p,
            size_t source_size,
            bool is_direct,
            bool is_strict,
            jerry_value_t *retval_p);
```

- `source_p` - source code to evaluate, it must be a valid utf8 string.
- `source_size` - length of the source code
- `is_direct` - whether to perform `eval` in "direct" mode (which means it is called as "eval" and
   not through some alias).
- `is_strict` - perform `eval` as it is called from "strict mode" code.
- `retval_p` - value, returned by `eval` (output parameter).
- return value - completion code that indicates whether run performed successfully. Same as before.

**Example**

```c
{
  jerry_value_t ret_val;

  jerry_completion_code_t status = jerry_eval (str_to_eval,
                                               strlen (str_to_eval),
                                               false,
                                               false,
                                               &ret_val);
}
```

**See also**

- [jerry_create_external_function](#jerry_create_external_function)
- [jerry_external_handler_t](#jerry_external_handler_t)


# jerry_create_undefined_value

**Summary**

Creates a `jerry_value_t` representing an undefined value. The value has to be released by
[jerry_release_value](#jerry_release_value).

**Prototype**

```c
jerry_value_t
jerry_create_undefined_value (void);
```

- return value - a `jerry_value_t` representing undefined.

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);
  jerry_value_t undefined_value = jerry_create_undefined_value ();

  ... // usage of the value

  jerry_release_value (undefined_value);
}
```

**See also**

- [jerry_release_value](#jerry_release_value)


# jerry_create_null_value

**Summary**

Creates a `jerry_value_t` representing a null value. The value has to be released by
[jerry_release_value](#jerry_release_value).

**Prototype**

```c
jerry_value_t
jerry_create_null_value (void);
```

- return value - a `jerry_value_t` representing null.

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);
  jerry_value_t null_value = jerry_create_null_value ();

  ... // usage of the value

  jerry_release_value (null_value);
}
```

**See also**

- [jerry_release_value](#jerry_release_value)


# jerry_create_boolean_value

**Summary**

Creates a `jerry_value_t` representing a boolean value from the given boolean parameter. The
value has to be released by [jerry_release_value](#jerry_release_value).

**Prototype**

```c
jerry_value_t
jerry_create_boolean_value (bool value);
```

- `value` - boolean value.
- return value - a `jerry_value_t` created from the given boolean argument.

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);
  jerry_value_t boolean_value = jerry_create_boolean_value (true);

  ... // usage of the value

  jerry_release_value (boolean_value);
}
```

**See also**

- [jerry_release_value](#jerry_release_value)


# jerry_create_number_value

**Summary**

Creates a `jerry_value_t` from the given double parameter. The value has to be released by
[jerry_release_value](#jerry_release_value).

**Prototype**

```c
jerry_value_t
jerry_create_number_value (double value);
```

- `value` - double value from which a `jerry_value_t` will be created.
- return value - a `jerry_value_t` created from the given double argument.

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);
  jerry_value_t number_value = jerry_create_number_value (3.14);

  ... // usage of the value

  jerry_release_value (number_value);
}
```

**See also**

- [jerry_release_value](#jerry_release_value)


# jerry_create_object_value

**Summary**

Converts the given `jerry_object_t` parameter to a `jerry_value_t`. The value has to be released
by [jerry_release_value](#jerry_release_value) when it is no longer needed.

**Prototype**

```c
jerry_value_t
jerry_create_object_value (jerry_object_t *value);
```

- `value` - `jerry_object_t` from which a `jerry_value_t` will be created.
- return value - a `jerry_value_t` created from the given `jerry_object_t` argument.

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);
  jerry_object_t *object_p = jerry_create_object ();
  jerry_value_t object_value = jerry_create_object_value (object_p);

  ... // usage of object_value

  jerry_release_value (object_value);
}
```

**See also**

- [jerry_release_value](#jerry_release_value)


# jerry_create_string_sz

**Summary**

Creates a string. The caller should release the string with [jerry_release_string](#jerry_release_string)
when it is no longer needed.

**Prototype**

```c
jerry_string_t *
jerry_create_string_sz (const jerry_char_t *str_p,
                        jerry_size_t str_size)
```

- `str_p` - string from which the result `jerry_string_t` will be created.
- `str_size` - size of the string.
- return value - a `jerry_string_t*` created from the given `jerry_char_t*` argument.

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);
  const jerry_char_t char_array[] = "a string";
  jerry_string_t *string  = jerry_create_string_sz (char_array,
                                                    strlen ((char *) char_array));

  ... // usage of string

  jerry_release_string (string);
}

```

**See also**

- [jerry_release_string](#jerry_release_string)


# jerry_create_string_value

**Summary**

Creates a `jerry_value_t` from the given `jerry_string_t` parameter. The value has to be released
by [jerry_release_value](#jerry_release_value) when it is no longer needed.

**Prototype**

```c
jerry_value_t
jerry_create_string_value (jerry_string_t *value);
```

- `value` - `jerry_string_t` from which a value will be created.
- return value - a `jerry_value_t` created from the given `jerry_string_t` argument.

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);
  const jerry_char_t char_array[] = "a string";
  jerry_string_t *string  = jerry_create_string_sz (char_array,
                                                    strlen ((char *) char_array));
  jerry_value_t string_value = jerry_create_string_value (string);

  ... // usage of string_value

  jerry_release_value (string_value);
}
```

**See also**

- [jerry_release_value](#jerry_release_value)


# jerry_create_error_sz

**Summary**

Creates an error object. Caller should release the object with jerry_release_object, when
the value is no longer needed.

**Prototype**

```c
jerry_object_t *
jerry_create_error_sz (jerry_error_t error_type,
                       const jerry_char_t *message_p,
                       jerry_size_t message_size);
```

- `error_type` - type of the error.
- `message_p` - value of 'message' property of the constructed error object.
- `message_size` - size of the message in bytes.
- return value - pointer to the created error object.

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);
  jerry_char_t message[] = "error";
  jerry_object_t *error_obj = jerry_create_error_sz (JERRY_ERROR_COMMON,
                                                     message,
                                                     strlen ((char *) message));

  ... // usage of error_obj

  jerry_release_object (error_obj);
}
```

**See also**

- [jerry_create_error](#jerry_create_error)


# jerry_get_boolean_value

**Summary**

Gets the raw bool value form a `jerry_value_t` value.

**Prototype**

```c
bool
jerry_get_boolean_value (const jerry_value_t value);
```

- `value` - api value
- return value - boolean value represented by the argument.

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);
  jerry_value_t value;
  ... // create or acquire value

  if (jerry_value_is_boolean (value))
  {
    bool raw_value = jerry_get_boolean_value (value);

    ... // usage of raw value

  }

  jerry_release_value (value);
}

```

**See also**

- [jerry_value_is_boolean](#jerry_value_is_boolean)
- [jerry_release_value](#jerry_release_value)


# jerry_get_number_value

**Summary**

Gets the number value of the given `jerry_value_t` parameter as a raw double.

**Prototype**

```c
double
jerry_get_number_value (const jerry_value_t value);
```

- `value` - api value
- return value - the number value of the given `jerry_value_t` parameter as a raw double.

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);
  jerry_value_t value;
  ... // create or acquire value

  if (jerry_value_is_number (value))
  {
    double raw_value = jerry_get_number_value (value);

    ... // usage of raw value

  }

  jerry_release_value (value);
}
```

**See also**

- [jerry_value_is_number](#jerry_value_is_number)
- [jerry_release_value](#jerry_release_value)


# jerry_get_object_value

**Summary**

Gets the object value of the given `jerry_value_t` parameter.

**Prototype**

```c
jerry_object_t *
jerry_get_object_value (const jerry_value_t value);
```

- `value` - api value
- return value - the object contained in the given `jerry_value_t` parameter.

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);
  jerry_value_t value;
  ... // create or acquire value

  if (jerry_value_is_object (value))
  {
    jerry_object_t *raw_value = jerry_get_object_value (value);

    ... // usage of raw value

  }

  jerry_release_value (value);
}
```

**See also**

- [jerry_value_is_object](#jerry_value_is_object)
- [jerry_release_value](#jerry_release_value)


# jerry_get_string_value

**Summary**

Gets the string value of the given `jerry_value_t` parameter.

**Prototype**

```c
jerry_string_t *
jerry_get_string_value (const jerry_value_t value);
```

- `value` - api value
- return value - the string contained in the given `jerry_value_t` parameter.

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);
  jerry_value_t value;
  ... // create or acquire value

  if (jerry_value_is_string (value))
  {
    jerry_string_t *raw_value = jerry_get_string_value (value);

    ... // usage of raw value

  }

  jerry_release_value (value);
}

```

**See also**

- [jerry_value_is_string](#jerry_value_is_string)
- [jerry_release_value](#jerry_release_value)


# jerry_get_string_size

**Summary**

Gets the length of a jerry_string_t.

**Prototype**

```c
jerry_size_t
jerry_get_string_size (const jerry_string_t *str_p);
```
- `str_p` - input string.
- return value - number of bytes in the buffer needed to represent the string.

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);
  jerry_char_t char_array[] = "a string";
  jerry_string_t *string = jerry_create_string (char_array);

  jerry_size_t string_size = jerry_get_string_size ();

  ... // usage of string_size

  jerry_release_string (string);
}  
```

**See also**

- [jerry_create_string](#jerry_create_string)
- [jerry_release_string](#jerry_release_string)
- [jerry_get_string_length](#jerry_get_string_length)


# jerry_get_string_length

**Summary**

Gets the length of a `jerry_string_t`.

**Prototype**

```c
jerry_length_t
jerry_get_string_length (const jerry_string_t *str_p);
```

- `str_p` - input string
- return value - number of characters in the string

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);
  jerry_char_t char_array[] = "a string";
  jerry_string_t *string = jerry_create_string (char_array);

  jerry_length_t string_length = jerry_get_string_length (string);

  ... // usage of string_length

  jerry_release_string (string);
}
```

**See also**

- [jerry_create_string](#jerry_create_string)
- [jerry_release_string](#jerry_release_string)
- [jerry_get_string_size](#jerry_get_string_size)


# jerry_value_is_undefined

**Summary**

Returns whether the given `jerry_value_t` is an undefined value.

**Prototype**

```c
bool
jerry_value_is_undefined (const jerry_value_t value);
```

- `value` - api value
- return value
  - true, if the given `jerry_value_t` is an undefined value
  - false, otherwise

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);
  jerry_value_t value;
  ... // create or acquire value

  if (jerry_value_is_undefined (value))
  {
    ...
  }

  jerry_release_value (value);
}
```

**See also**

- [jerry_release_value](#jerry_release_value)


# jerry_value_is_boolean

**Summary**

Returns whether the given `jerry_value_t` is a boolean value.

**Prototype**

```c
bool
jerry_value_is_boolean (const jerry_value_t value)
```

- `value` - api value
- return value
  - true, if the given `jerry_value_t` is a boolean value
  - false, otherwise

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);
  jerry_value_t value;
  ... // create or acquire value

  if (jerry_value_is_boolean (value))
  {
    ...
  }

  jerry_release_value (value);
}
```

**See also**

- [jerry_release_value](#jerry_release_value)


# jerry_value_is_number

**Summary**

Returns whether the given `jerry_value_t` is a number value.

**Prototype**

```c
bool
jerry_value_is_number (const jerry_value_t value);
```

- `value` - api value
- return value
  - true, if the given `jerry_value_t` is a number value
  - false, otherwise

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);
  jerry_value_t value;
  ... // create or acquire value

  if (jerry_value_is_number (value))
  {
    ...
  }

  jerry_release_value (value);
}
```

**See also**

- [jerry_release_value](#jerry_release_value)


# jerry_value_is_function

**Summary**

Returns whether the given `jerry_value_t` is a function object.

**Prototype**

```c
bool
jerry_value_is_function (const jerry_value_t value);
```

- `value` - api value
- return value
  - true, if the given `jerry_value_t` is a function object
  - false, otherwise

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);
  jerry_value_t value;
  ... // create or acquire value

  if (jerry_value_is_function (value))
  {
    ...
  }

  jerry_release_value (value);
}
```

**See also**

- [jerry_is_function](#jerry_is_function)
- [jerry_release_value](#jerry_release_value)


# jerry_value_is_null

**Summary**

Returns whether the given jerry_value_t is a null value.

**Prototype**

```c
bool
jerry_value_is_null (const jerry_value_t value);
```

- `value` - api value
- return value
  - true, if the given `jerry_value_t` is a null value
  - false, otherwise

**Example**

```c
  jerry_init (JERRY_FLAG_EMPTY);
  jerry_value_t value;
  ... // create or acquire value

  if (jerry_value_is_null (value))
  {
    ...
  }

  jerry_release_value (value);
```

**See also**

- [jerry_release_value](#jerry_release_value)


# jerry_value_is_object

**Summary**

Returns whether the given `jerry_value_t` is an object.

**Prototype**

```c
bool
jerry_value_is_object (const jerry_value_t value);
```

- `value` - api value
- return value
  - true, if the given `jerry_value_t` is an object value
  - false, otherwise

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);
  jerry_value_t value;
  ... // create or acquire value

  if (jerry_value_is_object (value))
  {
    ...
  }

  jerry_release_value (value);
}
```

**See also**

- [jerry_release_value](#jerry_release_value)


# jerry_value_is_string

**Summary**

Returns whether the given `jerry_value_t` is a string.

**Prototype**

```c
bool
jerry_value_is_string (const jerry_value_t value);
```

- `value` - api value
- return value
  - true, if the given `jerry_value_t` is a string
  - false, otherwise

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);
  jerry_value_t value;
  ... // create or acquire value

  if (jerry_value_is_string (value))
  {
    ...
  }

  jerry_release_value (value);
}
```

**See also**

- [jerry_release_value](#jerry_release_value)


# jerry_acquire_value

**Summary**

Acquires the specified Jerry API value.

For values of string, number and object types this acquires the underlying data, for all other types
it is a no-op. The acquired pointer has to be released by [jerry_release_value](#jerry_release_value).

**Prototype**

```c
jerry_value_t
jerry_acquire_value (jerry_value_t value);
```

- `value` - api value
- return value - pointer that may be used outside of the engine

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);
  jerry_object_t *object_p = jerry_create_object ();
  jerry_value_t object_value = jerry_create_object_value (object_p);

  jerry_value_t acquired_object = jerry_acquire_value (object_value);

  jerry_release_value (acquired_object);
  jerry_release_object (object_p);
}
```

**See also**

- [jerry_release_value](#jerry_release_value)
- [jerry_acquire_string](#jerry_acquire_string)
- [jerry_acquire_object](#jerry_acquire_object)


# jerry_release_value

**Summary**

Release specified pointer to the value.

**Prototype**

```c
void
jerry_release_value (jerry_value_t value);
```

- `value` - api value

**Example**

```c
{
    jerry_value_t val1 = jerry_create_object_value (jerry_create_object ());

    jerry_string_t *str_p = jerry_create_string ("abc");
    jerry_value_t val2 = jerry_create_string_value (str_p);

    ... // usage of val1

    jerry_release_value (val1);

    ... // usage of val2

    jerry_release_value (val2);
}
```


# jerry_create_string

**Summary**

Create new JavaScript string.

*Note*: Returned value must be freed with [jerry_release_object](#jerry_release_object) when it
is no longer needed.

**Prototype**

```c
jerry_string_t *
jerry_create_string (const jerry_char_t *v);
```

- `v` - value of string to create;
- return value - pointer to the created string.

**Example**

```c
{
  jerry_string_t *string_p = jerry_create_string ((jerry_char_t *) "abc");

  ...

  jerry_release_string (string_p);
}
```

**See also**

- [jerry_acquire_string](#jerry_acquire_string)
- [jerry_release_string](#jerry_release_string)
- [jerry_string_to_char_buffer](#jerry_string_to_char_buffer)


# jerry_string_to_char_buffer

**Summary**

Copy string characters to specified buffer. It is the caller's responsibility to make sure that
the string fits in the buffer.

*Note*: '\0' could occur in characters.

**Prototype**

```c
jerry_size_t
jerry_string_to_char_buffer (const jerry_string_t *string_p,
                             jerry_char_t *buffer_p,
                             jerry_size_t buffer_size);
```

- `string_p` - pointer to a C string.
- `buffer_p` - pointer to output buffer
- `buffer_size` - size of the buffer
- return value:
  - number of bytes, actually copied to the buffer.

**Example**

```c
{
  jerry_object_t *obj_p = jerry_get_global ();
  
  jerry_value_t val = jerry_get_object_field_value (obj_p,
                                                    "field_with_string_value");

  if (!jerry_value_is_error (val))
  {
    if (jerry_value_is_string (val))
    {
      char str_buf_p[128];
      jerry_string_t *str_p = jerry_get_string_value (val);
      jerry_size_t str_size = jerry_get_string_size (str_p);
      jerry_size_t sz = jerry_string_to_char_buffer (str_p, (jerry_char_t *) str_buf_p, str_size);
      str_buf_p[sz] = '\0';

      printf ("%s", str_buf_p);
    }

    jerry_release_value (val);
  }

  jerry_release_object (obj_p);
}
```

**See also**

- [jerry_create_string](#jerry_create_string)


# jerry_acquire_string

**Summary**

Acquire new pointer to the string for usage outside of the engine. The acquired pointer has to be
released with [jerry_release_string](#jerry_release_string).

**Prototype**

```c
jerry_string_t *
jerry_acquire_string (jerry_string_t *string_p);
```

- `string_p` - pointer to the string.
- return value - pointer to the string.

**Example**

```c
{
  jerry_string_t *str_ptr1_p = jerry_create_string ((jerry_char_t *) "abc");
  jerry_string_t *str_ptr2_p = jerry_acquire_string (str_ptr1_p);

  ... // usage of both pointers

  jerry_release_string (str_ptr1_p);

  ... // usage of str_ptr2_p pointer

  jerry_release_string (str_ptr2_p);
}
```

**See also**

- [jerry_release_string](#jerry_release_string)
- [jerry_create_string](#jerry_create_string)

# jerry_release_string

**Summary**

Release specified pointer to the string.

**Prototype**

```c
void
jerry_release_string (jerry_string_t *string_p);
```

- `string_p` - pointer to the string.

**Example**

```c
{
  jerry_string_t *str_ptr1_p = jerry_create_string ("abc");
  jerry_string_t *str_ptr2_p = jerry_acquire_string (str_ptr1_p);

  ... // usage of both pointers

  jerry_release_string (str_ptr1_p);

  ... // usage of str_ptr2_p pointer

  jerry_release_string (str_ptr2_p);
}
```

**See also**

- [jerry_acquire_string](#jerry_acquire_string)
- [jerry_create_string](#jerry_create_string)


# jerry_create_object

**Summary**

Create new JavaScript object, like with `new Object()`.

*Note*: Returned value must be freed with [jerry_release_object](#jerry_release_object) when it
is no longer needed.

**Prototype**

```c
jerry_object_t *
jerry_create_object ();
```

- return value - pointer to the created object.

**Example**

```c
{
  jerry_object_t *object_p = jerry_create_object ();

  ...

  jerry_release_object (object_p);
}
```

**See also**

- [jerry_acquire_object](#jerry_acquire_object)
- [jerry_release_object](#jerry_release_object)
- [jerry_add_object_field](#jerry_add_object_field)
- [jerry_delete_object_field](#jerry_delete_object_field)
- [jerry_get_object_field_value](#jerry_get_object_field_value)
- [jerry_set_object_field_value](#jerry_set_object_field_value)
- [jerry_get_object_native_handle](#jerry_get_object_native_handle)
- [jerry_set_object_native_handle](#jerry_set_object_native_handle)


# jerry_acquire_object

**Summary**

Acquire new pointer to the object for usage outside of the engine. The acquired pointer has to be
released with [jerry_release_object](#jerry_release_object).

**Prototype**

```c
jerry_object_t *
jerry_acquire_object (jerry_object_t *object_p);
```

- `object_p` - pointer to the object.
- return value - new pointer to the object.

**Example**

```c
{
  jerry_object_t *obj_ptr1_p = jerry_create_object ();
  jerry_object_t *obj_ptr2_p = jerry_acquire_object (obj_ptr1_p);

  ... // usage of both pointers

  jerry_release_object (obj_ptr1_p);

  ... // usage of obj_ptr2_p pointer

  jerry_release_object (obj_ptr2_p);
}
```

**See also**

- [jerry_release_object](#jerry_release_object)
- [jerry_create_object](#jerry_create_object)


# jerry_release_object

**Summary**

Release specified pointer to the object.

**Prototype**

```c
void
jerry_release_object (jerry_object_t *object_p);
```

- `object_p` - pointer to the object.

**Example**

```c
{
  jerry_object_t *obj_ptr1_p = jerry_create_object ();
  jerry_object_t *obj_ptr2_p = jerry_acquire_object (obj_ptr1_p);

  ... // usage of both pointers

  jerry_release_object (obj_ptr1_p);

  ... // usage of obj_ptr2_p pointer

  jerry_release_object (obj_ptr2_p);
}
```

**See also**

- [jerry_acquire_object](#jerry_acquire_object)
- [jerry_create_object](#jerry_create_object)


# jerry_get_global

**Summary**

Get the Global object.

**Prototype**

```c
jerry_object_t *
jerry_get_global (void);
```

- return value - pointer to the Global object.

Received pointer should be released with [jerry_release_object](#jerry_release_object), just
when the value is no longer needed.

**Example**

```c
{
  jerry_object_t *glob_obj_p = jerry_get_global ();

  jerry_value_t val = jerry_get_object_field_value (glob_obj_p, "some_field_name");
  if (!jerry_value_is_error (val))
  {
    ... // usage of 'val'
  }

  jerry_release_value (val);
  jerry_release_object (glob_obj_p);
}
```

**See also**

- [jerry_release_object](#jerry_release_object)
- [jerry_add_object_field](#jerry_add_object_field)
- [jerry_delete_object_field](#jerry_delete_object_field)
- [jerry_get_object_field_value](#jerry_get_object_field_value)
- [jerry_set_object_field_value](#jerry_set_object_field_value)


# jerry_add_object_field

**Summary**

Create field (named data property) in an object.

**Prototype**

```c
bool
jerry_add_object_field (jerry_object_t *object_p,
                        const jerry_char_t *field_name_p,
                        const jerry_value_t field_value,
                        bool is_writable);
```

- `object_p` - pointer to object to add field at.
- `field_name_p` - name of the field.
- `field_value` - value of the field.
- `is_writable` - flag indicating whether the created field should be writable.
- return value
  - true, if field was created successfully, i.e. upon the call:
    - there is no field with same name in the object;
    - the object is extensible.

**Example**

```c
{
  jerry_object_t *obj_p = jerry_create_object ();

  jerry_value_t val;

  ... // initialize val

  // Make new constant field
  jerry_add_object_field (obj_p, "some_field_name", val, false);
}
```

**See also**

- [jerry_create_object](#jerry_create_object)


# jerry_delete_object_field

**Summary**

Delete field (property) in the specified object

**Prototype**

```c
bool
jerry_delete_object_field (jerry_object_t *object_p,
                           const jerry_char_t *field_name_p);
```

- `object_p` - pointer to object to delete field at.
- `field_name_p` - name of the field.
- return value - true, if field was deleted successfully, i.e. upon the call:
  - there is field with specified name in the object.

**Example**

```c
{
  jerry_object_t *obj_p;
  ... // receive or construct obj_p

  jerry_delete_object_field (obj_p, "some_field_name");
}
```

**See also**

- [jerry_create_object](#jerry_create_object)


# jerry_get_object_field_value

**Summary**

Get value of field (property) in the specified object, i.e. perform [[Get]] operation.

**Prototype**

```c
jerry_value_t
jerry_get_object_field_value (jerry_object_t *object_p,
                              const jerry_char_t *field_name_p);
```

- `object_p` - pointer to object.
- `field_name_p` - name of the field.
- return value - jerry value of the given field.

*Note*: Returned value must be freed with [jerry_release_object](#jerry_release_object) when it
is no longer needed.

**Example**

```c
{
  jerry_object_t *obj_p;
  ... // receive or construct obj_p

  jerry_value_t val = jerry_get_object_field_value (obj_p, "some_field_name");

  if (!jerry_value_is_error (val))
  {
    ... // usage of 'val'

  }

  jerry_release_value (val);
}
```

**See also**

- [jerry_create_object](#jerry_create_object)
- [jerry_set_object_field_value](#jerry_set_object_field_value)
- [jerry_get_object_field_value_sz](#jerry_get_object_field_value_sz)


# jerry_get_object_field_value_sz

**Summary**

Gets the value of a field in the specified object.

**Prototype**

```c
jerry_value_t
jerry_get_object_field_value_sz (jerry_object_t *object_p,
                                 const jerry_char_t *field_name_p,
                                 jerry_size_t field_name_size);
```

- `object_p` - pointer to object.
- `field_name_p` - name of the field.
- `field_name_size` - size of field name in bytes.
- return value - jerry value of the given field.

*Note*: Returned value must be freed with [jerry_release_object](#jerry_release_object) when it
is no longer needed.

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);

  jerry_char_t field_name[] = "field";
  jerry_value_t field_value = jerry_create_number_value (3.14);

  jerry_object_t* object_p = jerry_create_object ();
  jerry_add_object_field (object_p,
                          field_name,
                          strlen ((char *) field_name),
                          field_value, 
                          true);

  jerry_value_t retrieved_field_value;
  retrieved_field_value = jerry_get_object_field_value_sz (object_p,
                                                           field_name,
                                                           strlen ((char *) field_name));
  if (!jerry_value_is_error (retrieved_field_value))
  {
    ... // usage of the retrieved field
  }

  jerry_release_value (retrieved_field_value);
  jerry_release_object (object_p);
}
```

**See also**

- [jerry_init](#jerry_init)
- [jerry_create_object](#jerry_create_object)
- [jerry_release_object](#jerry_release_object)
- [jerry_add_object_field](#jerry_add_object_field)
- [jerry_get_object_field_value](#jerry_get_object_field_value)
- [jerry_set_object_field_value](#jerry_set_object_field_value)
- [jerry_set_object_field_value_sz](#jerry_set_object_field_value_sz)


# jerry_set_object_field_value

**Summary**

Set value of a field (property) in the specified object, i.e. perform [[Put]] operation.

**Prototype**

```c
bool
jerry_set_object_field_value (jerry_object_t *object_p,
                              const jerry_char_t *field_name_p,
                              jerry_value_t field_value);
```

- `object_p` - pointer to object.
- `field_name_p` - name of the field.
- `field_value` - field value to set.
- return value - true, if field value was set successfully, i.e. upon the call:
  - field value is writable.

**Example**

```c
{
  jerry_object_t *obj_p;
  jerry_value_t val;

  ... // receive or construct obj_p and val

  bool is_ok = jerry_set_object_field_value (obj_p,
                                             (const jerry_char_t *) "some_field_name",
                                             val);
}
```

**See also**

- [jerry_create_object](#jerry_create_object)
- [jerry_get_object_field_value](#jerry_get_object_field_value)
- [jerry_get_object_field_value_sz](#jerry_set_object_field_value_sz)
- [jerry_set_object_field_value_sz](#jerry_set_object_field_value_sz)


# jerry_set_object_field_value_sz

**Summary**

Set value of field in the specified object.

**Prototype**

```c
bool
jerry_set_object_field_value_sz (jerry_object_t *object_p,
                                 const jerry_char_t *field_name_p,
                                 jerry_size_t field_name_size,
                                 const jerry_value_t field_value);
```

- `object_p` - pointer to object
- `field_name_p` - name of the field
- `field_name_size` - size of field name in bytes
- `field_value` - field value to set
- return value
 - true, if field value was set successfully
 - false, otherwise

**Example**

```c
{
  jerry_object_t *obj_p;
  jerry_char_t field_name[] = "some_field_name";
  jerry_value_t val;

  ... // receive or construct obj_p and val

  bool is_ok = jerry_set_object_field_value_sz (obj_p,
                                                field_name,
                                                strlen ((char *) field_name),
                                                val);
}
```

**See also**

- [jerry_create_object](#jerry_create_object)
- [jerry_get_object_field_value](#jerry_get_object_field_value)
- [jerry_get_object_field_value_sz](#jerry_set_object_field_value_sz)
- [jerry_set_object_field_value](#jerry_set_object_field_value)


# jerry_get_object_native_handle

**Summary**

Get native handle, previously associated with specified object.

**Prototype**

```c
bool
jerry_get_object_native_handle (jerry_object_t *object_p,
                                uintptr_t *out_handle_p);
```

- `object_p` - pointer to object to get handle from.
- `out_handle_p` - handle value (output parameter).
- return value - true, if there is handle associated with the object.

**Example**

```c
{
  jerry_object_t *obj_p;
  uintptr_t handle_set;

  ... // receive or construct obj_p and handle_set value

  jerry_set_object_native_handle (obj_p, handle_set, NULL);

  ...

  uintptr_t handle_get;
  bool is_there_associated_handle = jerry_get_object_native_handle (obj_p, &handle_get);
}
```

**See also**

- [jerry_create_object](#jerry_create_object)
- [jerry_set_object_native_handle](#jerry_set_object_native_handle)

# jerry_set_object_native_handle

**Summary**

Set native handle and, optionally, "free" callback for the specified object.

If native handle or "free" callback were already set for the object, corresponding value is updated.

**Prototype**

```c
void
jerry_set_object_native_handle (jerry_object_t *object_p,
                                uintptr_t handle,
                                jerry_object_free_callback_t freecb_p);
```

- `object_p` - pointer to object to set handle in;
- `handle` - handle value;
- `freecb_p` - pointer to "free" callback or NULL (if not NULL the callback would be called upon
  GC of the object).

**Example**

```c
{
  jerry_object_t *obj_p;
  uintptr_t handle_set;

  ... // receive or construct obj_p and handle_set value

  jerry_set_object_native_handle (obj_p, handle_set, NULL);

  ...

  uintptr_t handle_get;
  bool is_there_associated_handle = jerry_get_object_native_handle (obj_p, &handle_get);
}
```

**See also**

- [jerry_create_object](#jerry_create_object)
- [jerry_get_object_native_handle](#jerry_get_object_native_handle)

# jerry_is_function

**Summary**

Check whether the specified object is a function object.

**Prototype**

```c
bool
jerry_is_function (const jerry_object_t *object_p);
```

- `object_p` - pointer to object to check;
- return value - boolean, indicating whether the specified object can be called as function.

**Example**

```c
{
  jerry_value_t val;

  ... // receiving val

  if (jerry_value_is_object (val))
  {
    if (jerry_is_function (jerry_get_object_value (val)))
    {
      // the object is function object
    }
  }
}
```

**See also**

- [jerry_is_constructor](#jerry_is_constructor)
- [jerry_call_function](#jerry_call_function)


# jerry_is_constructor

**Summary**

Check whether the specified object is a constructor function object.

**Prototype**

```c
bool
jerry_is_constructor (const jerry_object_t *object_p);
```

- `object_p` - pointer to object to check.
- return value - boolean, indicating whether the specified object can be called as constructor.

**Example**

```c
{
  jerry_value_t val;

  ... // receiving val

  if (jerry_value_is_object (val))
  {
    if (jerry_is_constructor (jerry_get_object_value (val)))
    {
      // the object is constructor function object
    }
  }
}
```

**See also**

- [jerry_is_function](#jerry_is_function)
- [jerry_construct_object](#jerry_construct_object)


# jerry_call_function

**Summary**

Call function object.

**Prototype**

```c
jerry_value_t
jerry_call_function (jerry_object_t *function_object_p,
                     jerry_object_t *this_arg_p,
                     const jerry_value_t args_p[],
                     uint16_t args_count);
```

- `function_object_p` - the function object to call;
- `this_arg_p` - object to use as 'this' during the invocation, or NULL - to set the Global object
  as 'this';
- `args_p`, `args_count` - array of arguments and number of them;
- return value - function's return value

Returned value must be freed with [jerry_release_object](#jerry_release_object) when it is no
longer needed.

**Example**

```c
{
  jerry_value_t val;

  ... // receiving val

  if (jerry_value_is_object (val))
  {
    if (jerry_is_function (jerry_get_object_value (val)))
    {
      jerry_value_t ret_val;

      ret_val = jerry_call_function (jerry_get_object_value (val), NULL, NULL, 0);

      if (!jerry_value_is_error (ret_val))
      {
        ... // handle return value
      }

      jerry_release_value (ret_val);
    }
  }
}
```

**See also**

- [jerry_is_function](#jerry_is_function)
- [jerry_create_external_function](#jerry_create_external_function)


# jerry_construct_object

**Summary**

Construct object, invoking specified function object as constructor.

**Prototype**

```c
jerry_value_t
jerry_construct_object (jerry_object_t *function_object_p,
                        const jerry_value_t args_p[],
                        uint16_t args_count);
```

- `function_object_p` - the function object to invoke.
- `args_p`, `args_count` - array of arguments and number of them.
- return value - return value of function invoked as constructor, i.e. like with 'new' operator.

*Note*: Returned value must be freed with [jerry_release_object](#jerry_release_object) when
it is no longer needed.

**Example**

```c
{
  jerry_value_t val;

  ... // receiving val

  if (jerry_value_is_object (val))
  {
    if (jerry_is_function (jerry_get_object_value (val)))
    {
      jerry_value_t ret_val;

      ret_val = jerry_construct_object (jerry_get_object_value (val), NULL, 0);

      if (!jerry_value_is_error (ret_val))
      {
        ... // handle return value
      }

      jerry_release_value (ret_val);
    }
  }
}
```

**See also**

 - [jerry_is_constructor](#jerry_is_constructor)


# jerry_external_handler_t

**Summary**

The data type represents pointer to call handler of a native function object.

**Structure**

```c
typedef bool (* jerry_external_handler_t) (const jerry_object_t *function_obj_p,
                                           const jerry_value_t this_p,
                                           const jerry_value_t args_p[],
                                           const uint16_t args_count,
                                           jerry_value_t *ret_val_p);
```

**See also**

- [jerry_create_external_function](#jerry_create_external_function)


# jerry_create_external_function

**Summary**

Create an external function object.

**Prototype**

```c
jerry_object_t *
jerry_create_external_function (jerry_external_handler_t handler_p);
```

- `handler_p` - pointer to native handler of the function object;
- return value - pointer to constructed external function object.

Received pointer should be released with [jerry_release_object](#jerry_release_object), when
the value is no longer needed.

**Example**

```c
static bool
handler (const jerry_object_t *function_obj_p,
         const jerry_value_t this_p,
         const jerry_value_t args_p[],
         const uint16_t args_cnt,
         jerry_value_t *ret_val_p)
{
  printf ("native handler called!\n");

  *ret_val_p = jerry_create_boolean_value (true);
  return true;
}

{
  jerry_object_t *obj_p = jerry_create_external_function (handler);
  jerry_object_t *glob_obj_p = jerry_get_global ();

  jerry_value_t val = jerry_create_object_value (obj_p);

  // after this, script can invoke the native handler through "handler_field (1, 2, 3);"
  jerry_set_object_field_value (glob_obj_p, "handler_field", val);

  jerry_release_object (glob_obj_p);
  jerry_release_object (obj_p);
}
```

**See also**

- [jerry_external_handler_t](#jerry_external_handler_t)
- [jerry_is_function](#jerry_is_function)
- [jerry_call_function](#jerry_call_function)
- [jerry_release_object](#jerry_release_object)


# jerry_create_array_object

**Summary**

Create new JavaScript array object.

*Note*: Returned value must be freed with [jerry_release_object](#jerry_release_object) when it
is no longer needed.

**Prototype**

```c
jerry_object_t *
jerry_create_array_object (jerry_size_t array_size);
```

 - `array_size` - size of array;
 - return value is pointer to the created array object.

 **Example**

```c
{
    jerry_object_t *array_object_p = jerry_create_array_object (10);

    ...

    jerry_release_object (array_object_p);
}
```

**See also**

- [jerry_acquire_object](#jerry_acquire_object)
- [jerry_release_object](#jerry_release_object)
- [jerry_set_array_index_value](#jerry_set_array_index_value)
- [jerry_get_array_index_value](#jerry_get_array_index_value)
- [jerry_add_object_field](#jerry_add_object_field)
- [jerry_delete_object_field](#jerry_delete_object_field)
- [jerry_get_object_field_value](#jerry_get_object_field_value)
- [jerry_set_object_field_value](#jerry_set_object_field_value)
- [jerry_get_object_native_handle](#jerry_get_object_native_handle)
- [jerry_set_object_native_handle](#jerry_set_object_native_handle)


# jerry_set_array_index_value

**Summary**

Set value of an indexed element in the specified array object.

**Prototype**

```c
bool
jerry_set_array_index_value (jerry_object_t *array_object_p,
                             jerry_length_t index,
                             jerry_value_t value);
```

- `array_object_p` - pointer to the array object;
- `index` - index of the array element;
- `value` - value to set;
- return value - true, if value was set successfully.

**Example**

```c
{
    jerry_object_t *array_object_p = jerry_create_array_object (10);
    jerry_value_t val;

    ... // receive or construct val

    jerry_set_array_index_value (array_object_p, 5, val);

    jerry_release_object (array_object_p);
}
```

**See also**

- [jerry_create_array_object](#jerry_create_array_object)


# jerry_get_array_index_value

**Summary**

Get value of an indexed element in the specified array object.

**Prototype**

```c
bool
jerry_get_array_index_value (jerry_object_t *array_object_p,
                             jerry_length_t index,
                             jerry_value_t *value_p);
```

- `array_object_p` - pointer to the array object;
- `index` - index of the array element;
- `value_p` - retrieved indexed value (output parameter);
- return value - true, if value was retrieved successfully.

**Example**

```c
{
    jerry_object_t *array_object_p;
    ... // receive or construct array_object_p

    jerry_value_t val;
    bool is_ok = jerry_get_array_index_value (array_object_p, 5, &val);
    if (is_ok)
    {
      ... // usage of 'val'
    }
}
```

**See also**

- [jerry_create_array_object](#jerry_create_array_object)


# jerry_create_error

**Summary**

Create new JavaScript error object. It should be throwed from a handle attached to an external
function object.

**Prototype**

```c
jerry_object_t *
jerry_create_error (jerry_error_t error_type,
                    const jerry_char_t *message_p);
```

- `error_type` - error type of object;
- `message_p` - human-readable description of the error;
- return value is pointer to the created error object.

**Example**

```c
static bool
handler (const jerry_object_t *function_obj_p,
         const jerry_value_t this_p,
         const jerry_value_t args_p[],
         const uint16_t args_cnt,
         jerry_value_t *ret_val_p)
{
  jerry_object_t *error_p = jerry_create_error (JERRY_ERROR_TYPE,
                                                (jerry_char_t * ) "error");

  jerry_acquire_object (error_p);
  *ret_val_p = jerry_create_error_value (error_p);

  jerry_release_object (error_p);

  return false;
}

{
  jerry_object_t *throw_obj_p = jerry_create_external_function (handler);
  jerry_object_t *glob_obj_p = jerry_get_global ();

  jerry_value_t val = jerry_create_object_value (throw_obj_p);

  // after this, script can invoke the native handler through "error_func ();"
  // and "error_func" will throw an error when called.
  jerry_set_object_field_value (glob_obj_p, "error_func", val);

  jerry_release_object (glob_obj_p);
  jerry_release_object (throw_obj_p);
}
```

**See also**

- [jerry_external_handler_t](#jerry_external_handler_t)
- [jerry_is_function](#jerry_is_function)
- [jerry_call_function](#jerry_call_function)
- [jerry_release_object](#jerry_release_object)
- [jerry_create_external_function](#jerry_create_external_function)


# jerry_register_external_magic_strings

**Summary**

Registers an external magic string array.

**Prototype**

```c
void
jerry_register_external_magic_strings  (const jerry_char_ptr_t *ex_str_items,
                                        uint32_t  count,
                                        const jerry_length_t *str_lengths);
```

- `ex_str_items` - character arrays, representing external magic strings' contents
- `count number` - of the strings
- `str_lengths` - lengths of the strings

**Example**

```c
{
  jerry_init (JERRY_FLAG_EMPTY);

  // must be static, because 'jerry_register_external_magic_strings' does not copy
  static const jerry_char_ptr_t magic_string_items[] = {
                                                         (const jerry_char_ptr_t) "magicstring1",
                                                         (const jerry_char_ptr_t) "magicstring2",
                                                         (const jerry_char_ptr_t) "magicstring3"
                                                        };
  uint32_t num_magic_string_items = (uint32_t) (sizeof (magic_string_items) / sizeof (jerry_char_ptr_t));

  // must be static, because 'jerry_register_external_magic_strings' does not copy
  static const jerry_length_t magic_string_lengths[] = {
                                                         (jerry_length_t)strlen (magic_string_items[0]),
                                                         (jerry_length_t)strlen (magic_string_items[1]),
                                                         (jerry_length_t)strlen (magic_string_items[2])
                                                       };
  jerry_register_external_magic_strings (magic_string_items,
                                         num_magic_string_items,
                                         magic_string_lengths);
}
```

**See also**

- [jerry_init](#jerry_init)


# jerry_value_to_string

**Summary**

Creates the textual representation of an API value using the ToString ecma builtin operation.

**Prototype**

```c
jerry_value_t
jerry_value_to_string (const jerry_value_t value);
```

- `value` - api value.
- return value - textual representation of the given value.

**Example**

```c
{
  jerry_value_t value;

  ... // receive or construct value

  jerry_value_t to_string_value = jerry_value_to_string (&value);

  jerry_release_value (to_string_value);
}
```


# jerry_object_field_foreach_t

**Summary**

Function type applied for each fields of an object by API function [jerry_foreach_object_field](#jerry_foreach_object_field).

**Definition**

```c
typedef bool (*jerry_object_field_foreach_t) (const jerry_string_t *field_name_p,
                                              const jerry_value_t field_value,
                                              void *user_data_p);
```

- `field_name_p` - name of the field
- `field_value` - value of the field
- `user_data_p` - user data
- return value
  - true, if the operation executed successfully
  - false, otherwise

**See also**

- [jerry_foreach_object_field](#jerry_foreach_object_field)


# jerry_foreach_object_field

**Summary**

Applies the given function to every fields in the given objects.

**Prototype**

```c
bool
jerry_foreach_object_field (jerry_object_t *object_p,
                            jerry_object_field_foreach_t foreach_p,
                            void *user_data_p);
```

- `object_p` - pointer to object
- `foreach_p` - foreach function, that will be apllied for each fields
- `user_data_p` - user data for foreach function
- return value
  - true, if object fields traversal was performed successfully, i.e.:
    - no unhandled exceptions were thrown in object fields traversal
    - object fields traversal was stopped on callback that returned false
  - false, otherwise, if getter of field threw a exception or unhandled exceptions were thrown
    during traversal

**Example**

```c
bool foreach_function (const jerry_string_t *field_name_p,
                       const jerry_value_t field_value,
                       void *user_data_p)
{

  ... // implementation of the foreach function

}

{
  jerry_init (JERRY_FLAG_EMPTY);

  jerry_object_t* object_p;
  ... // receive or construct object_p

  double data = 3.14; // example data

  jerry_foreach_object_field (object_p, foreach_function, &data);

}
```

**See also**

- [jerry_object_field_foreach_t](#jerry_object_field_foreach_t)
