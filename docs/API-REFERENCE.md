# jerry_run_simple

**Summary**
The simplest way to run JavaScript.

**Prototype**

```cpp
jerry_completion_code_t
jerry_run_simple (const char * script_source,
                  size_t script_source_size,
                  jerry_flag_t flags);
```

- `script_source` - source code;
- `script_source_size` - size of source code buffer, in bytes;
- returned value - completion code that indicates whether run was performed successfully (`JERRY_COMPLETION_CODE_OK`), or an unhandled JavaScript exception occurred (`JERRY_COMPLETION_CODE_UNHANDLED_EXCEPTION`).

**Example**

```cpp
{
  const char * script = "print ('Hello, World!');";

  jerry_run_simple (script, strlen (script), JERRY_FLAG_EMPTY);
}
```

**See also**

- [jerry_init](#jerryinit)
- [jerry_cleanup](#jerrycleanup)
- [jerry_parse](#jerryparse)
- [jerry_run](#jerryrun)

# jerry_init

**Summary**

Initializes JerryScript engine, making possible to run JavaScript code and perform operations on JavaScript values.

**Prototype**

```cpp
void
jerry_init (jerry_flag_t flags);
```

`flags` - combination of various engine configuration flags:

- `JERRY_FLAG_MEM_STATS` - dump memory statistics;
- `JERRY_FLAG_ENABLE_LOG` - enable logging;
- `JERRY_FLAG_SHOW_OPCODES` - print compiled byte-code;
- `JERRY_FLAG_EMPTY` - no flags, just initialize in default configuration.

**Example**

```cpp
{
  jerry_init (JERRY_FLAG_ENABLE_LOG);

  // ...

  jerry_cleanup ();
}
```

**See also**

- [jerry_cleanup](#jerrycleanup)

# jerry_cleanup

**Summary**

Finish JavaScript engine execution, freeing memory and JavaScript values.

JavaScript values, received from engine, are inaccessible after the cleanup.

**Prototype**

```cpp
void
jerry_cleanup (void);
```

**See also**

- [jerry_init](#jerryinit)

# jerry_parse

**Summary**
Parse specified script to execute in Global scope.

Current API doesn't permit replacement or modification of Global scope's code without engine restart,
so `jerry_parse` could be invoked only once between `jerry_init` and `jerry_cleanup`.

**Prototype**

```cpp
bool
jerry_parse (const char* source_p, size_t source_size);
```
- `source_p` - string, containing source code to parse;
- `source_size` - size of the string, in bytes.

**Example**

```cpp
{
  jerry_init (JERRY_FLAG_ENABLE_LOG);

  char script [] = "print ('Hello, World!');";
  jerry_parse (script, strlen (script));

  jerry_run ();

  jerry_cleanup ();
}
```

**See also**

- [jerry_run](#jerryrun)

# jerry_run

**Summary**
Run code of Global scope.

The code should be previously registered through `jerry_parse`.

**Prototype**

```cpp
jerry_completion_code_t
jerry_run (void);
```

- returned value - completion code that indicates whether run performed successfully (`JERRY_COMPLETION_CODE_OK`), or an unhandled JavaScript exception occurred (`JERRY_COMPLETION_CODE_UNHANDLED_EXCEPTION`).

**Example**

```cpp
{
  jerry_init (JERRY_FLAG_ENABLE_LOG);

  char script [] = "print ('Hello, World!');";
  jerry_parse (script, strlen (script));

  jerry_run ();

  jerry_cleanup ();
}
```

**See also**

- [jerry_parse](#jerryparse)

# jerry_api_value_t

**Summary**
The data type represents any JavaScript value that can be sent to or received from the engine.

Type of value is identified by `jerry_api_value_t::type`, and can be one of the following:

- `JERRY_API_DATA_TYPE_UNDEFINED` - JavaScript undefined;
- `JERRY_API_DATA_TYPE_NULL` - JavaScript null;
- `JERRY_API_DATA_TYPE_BOOLEAN` - boolean;
- `JERRY_API_DATA_TYPE_FLOAT64` - number;
- `JERRY_API_DATA_TYPE_STRING` - string;
- `JERRY_API_DATA_TYPE_OBJECT` - reference to JavaScript object.

**Structure**

```cpp
typedef struct jerry_api_value_t
{
  jerry_api_data_type_t type;

  union
  {
    bool v_bool;

    float v_float32;
    double v_float64;

    uint32_t v_uint32;

    union
    {
      jerry_api_string_t * v_string;
      jerry_api_object_t * v_object;
    };
  };
} jerry_api_value_t;
```

**See also**

- [jerry_api_eval](#jerryapieval)
- [jerry_api_call_function](#jerryapicallfunction)
- [jerry_api_construct_object](#jerryapiconstructobject)


# jerry_api_eval

**Summary**
Perform JavaScript `eval`.

**Prototype**

```cpp
jerry_completion_code_t
jerry_api_eval (const char * source_p,
                size_t source_size,
                bool is_direct,
                bool is_strict,
                jerry_api_value_t * retval_p);
```

- `source_p` - source code to evaluate;
- `source_size` - length of the source code;
- `is_direct` - whether to perform `eval` in "direct" mode (possible only if `eval` invocation is performed from native function, called from JavaScript);
- `is_strict` - perform `eval` as it is called from "strict mode" code;
- `retval_p` - value, returned by `eval` (output parameter);
- returned value - completion code that indicates whether run performed successfully (`JERRY_COMPLETION_CODE_OK`), or an unhandled JavaScript exception occurred (`JERRY_COMPLETION_CODE_UNHANDLED_EXCEPTION`).

**Example**

```cpp
{
  jerry_api_value_t ret_val;

  jerry_completion_code_t status = jerry_api_eval (str_to_eval,
                                                   strlen (str_to_eval),
                                                   false, false,
                                                   &ret_val);
}
```

**See also**

- [jerry_api_value_t](#jerryapivaluet)
- [jerry_api_create_external_function](#jerryapicreateexternalfunction)
- [jerry_external_handler_t](#jerryexternalhandlert)

# jerry_api_create_string

**Summary**
Create new JavaScript string.

Upon the JavaScript string becomes unused, all pointers to it should be released using [jerry_api_release_string](#jerryapireleasestring).

**Prototype**

```cpp
jerry_api_string_t*
jerry_api_create_string (const char * v);
```

- `v` - value of string to create;
- returned value is pointer to created string.

**Example**

```cpp
{
  jerry_api_string_t * string_p = jerry_api_create_string ("abc");

  ...

  jerry_api_release_string (string_p);
}
```

**See also**

- [jerry_api_acquire_string](#jerryapiacquirestring)
- [jerry_api_release_string](#jerryapireleasestring)
- [jerry_api_string_to_char_buffer](#jerryapistringtocharbuffer)

# jerry_api_string_to_char_buffer

**Summary**
Copy string characters to specified buffer, append zero character at end of the buffer.

**Prototype**

```cpp
ssize_t
jerry_api_string_to_char_buffer (const jerry_api_string_t * string_p,
                                 char * buffer_p,
                                 ssize_t buffer_size);
```

- `string_p` - pointer to a string;
- `buffer_p` - pointer to output buffer (can be NULL, is `buffer_size` is 0);
- `buffer_size` - size of the buffer;
- returned value:
  - number of bytes, actually copied to the buffer - if characters were copied successfully;
  - otherwise (in case size of buffer is insufficient) - negative number, which is calculated as negation of buffer size, that is required to hold characters.

**Example**

```cpp
{
  jerry_api_object_t * obj_p = jerry_api_get_global ();
  jerry_api_value_t val;

  bool is_ok = jerry_api_get_object_field_value (obj_p,
                                                 "field_with_string_value",
                                                 &val);

  if (is_ok) {
    bool is_string = (val.type == JERRY_API_DATA_TYPE_STRING);

    if (is_string) {
      // neg_req_sz would be negative, as zero-size buffer is insufficient for any string
      ssize_t neg_req_sz = jerry_api_string_to_char_buffer (val.string_p,
                                                            NULL,
                                                            0);
      char * str_buf_p = (char*) malloc (-neg_req_sz);

      // sz would be -neg_req_sz
      size_t sz = jerry_api_string_to_char_buffer (val.string_p,
                                                   str_buf_p,
                                                   -neg_req_sz);

      printf ("%s", str_buf_p);

      free (str_buf_p);
    }

    jerry_api_release_value (&val);
  }

  jerry_api_release_object (obj_p);
}
```

**See also**

- [jerry_api_create_string](#jerryapicreatestring)
- [jerry_api_value_t](#jerryapivaluet)

# jerry_api_acquire_string

**Summary**
Acquire new pointer to the string for usage outside of the engine.

The acquired pointer should be released with [jerry_api_release_string](#jerryapireleasestring).

**Prototype**

```cpp
jerry_api_string_t*
jerry_api_acquire_string (jerry_api_string_t * string_p);
```

- `string_p` - pointer to the string;
- returned value - new pointer to the string.

**Example**

```cpp
{
  jerry_api_string_t * str_ptr1_p = jerry_api_create_string ("abc");
  jerry_api_string_t * str_ptr2_p = jerry_api_acquire_string (str_ptr1_p);

  ... // usage of both pointers

  jerry_api_release_string (str_ptr1_p);

  ... // usage of str_ptr2_p pointer

  jerry_api_release_string (str_ptr2_p);
}
```

**See also**

- [jerry_api_release_string](#jerryapireleasestring)
- [jerry_api_create_string](#jerryapicreatestring)

# jerry_api_release_string

**Summary**
Release specified pointer to the string.

**Prototype**

```cpp
void
jerry_api_release_string (jerry_api_string_t * string_p);
```

- `string_p` - pointer to the string.

**Example**

```cpp
{
  jerry_api_string_t * str_ptr1_p = jerry_api_create_string ("abc");
  jerry_api_string_t * str_ptr2_p = jerry_api_acquire_string (str_ptr1_p);

  ... // usage of both pointers

  jerry_api_release_string (str_ptr1_p);

  ... // usage of str_ptr2_p pointer

  jerry_api_release_string (str_ptr2_p);
}
```

**See also**

- [jerry_api_acquire_string](#jerryapiacquirestring)
- [jerry_api_create_string](#jerryapicreatestring)

# jerry_api_create_object

**Summary**
Create new JavaScript object, like with `new Object()`.

Upon the JavaScript object becomes unused, all pointers to it should be released using [jerry_api_release_object](#jerryapireleaseobject).

**Prototype**

```cpp
jerry_api_object_t*
jerry_api_create_object ();
```

- returned value is pointer to the created object.

**Example**

```cpp
{
  jerry_api_object_t * object_p = jerry_api_create_object ();

  ...

  jerry_api_release_object (object_p);
}
```

**See also**

- [jerry_api_acquire_object](#jerryapiacquireobject)
- [jerry_api_release_object](#jerryapireleaseobject)
- [jerry_api_add_object_field](#jerryapiaddobjectfield)
- [jerry_api_delete_object_field](#jerryapideleteobjectfield)
- [jerry_api_get_object_field_value](#jerryapigetobjectfieldvalue)
- [jerry_api_set_object_field_value](#jerryapisetobjectfieldvalue)
- [jerry_api_get_object_native_handle](#jerryapigetobjectnativehandle)
- [jerry_api_set_object_native_handle](#jerryapisetobjectnativehandle)

# jerry_api_acquire_object

**Summary**
Acquire new pointer to the object for usage outside of the engine.

The acquired pointer should be released with [jerry_api_release_object](#jerryapireleaseobject).

**Prototype**

```cpp
jerry_api_object_t*
jerry_api_acquire_object (jerry_api_object_t * object_p);
```

- `object_p` - pointer to the object;
- returned value - new pointer to the object.

**Example**

```cpp
{
  jerry_api_object_t * obj_ptr1_p = jerry_api_create_object ();
  jerry_api_object_t * obj_ptr2_p = jerry_api_acquire_object (obj_ptr1_p);

  ... // usage of both pointers

  jerry_api_release_object (obj_ptr1_p);

  ... // usage of obj_ptr2_p pointer

  jerry_api_release_object (obj_ptr2_p);
}
```

**See also**

- [jerry_api_release_object](#jerryapireleaseobject)
- [jerry_api_create_object](#jerryapicreateobject)

# jerry_api_release_object

**Summary**
Release specified pointer to the object.

**Prototype**

```cpp
void
jerry_api_release_object (jerry_api_object_t * object_p);
```

- `object_p` - pointer to the object.

**Example**

```cpp
{
  jerry_api_object_t * obj_ptr1_p = jerry_api_create_object ();
  jerry_api_object_t * obj_ptr2_p = jerry_api_acquire_object (obj_ptr1_p);

  ... // usage of both pointers

  jerry_api_release_object (obj_ptr1_p);

  ... // usage of obj_ptr2_p pointer

  jerry_api_release_object (obj_ptr2_p);
}
```

**See also**

- [jerry_api_acquire_object](#jerryapiacquireobject)
- [jerry_api_create_object](#jerryapicreateobject)

# jerry_api_get_global

**Summary**
Get the Global object.

**Prototype**

```cpp
jerry_api_object_t*
jerry_api_get_global (void);
```

- returned value - pointer to the Global object.

Received pointer should be released with [jerry_api_release_object](#jerryapireleaseobject), just when the value becomes unnecessary.

**Example**

```cpp
{
  jerry_api_object_t * glob_obj_p = jerry_api_get_global ();

  jerry_api_value_t val;
  bool is_ok = jerry_api_get_object_field_value (glob_obj_p, "some_field_name", &val);
  if (is_ok)
  {
    ... // usage of 'val'

    jerry_api_release_value (&val);
  }

  jerry_api_release_object (glob_obj_p);
}
```

**See also**

- [jerry_api_release_object](#jerryapireleaseobject)
- [jerry_api_add_object_field](#jerryapiaddobjectfield)
- [jerry_api_delete_object_field](#jerryapideleteobjectfield)
- [jerry_api_get_object_field_value](#jerryapigetobjectfieldvalue)
- [jerry_api_set_object_field_value](#jerryapisetobjectfieldvalue)

# jerry_api_add_object_field

**Summary**
Create field (named data property) in an object

**Prototype**

```cpp
bool
jerry_api_add_object_field (jerry_api_object_t * object_p,
                            const char * field_name_p,
                            const jerry_api_value_t * field_value_p,
                            bool is_writable);
```

- `object_p` - object to add field at;
- `field_name_p` - name of the field;
- `field_value_p` - value of the field;
- `is_writable` - flag indicating whether the created field should be writable.
- returned value - true, if field was created successfully, i.e. upon the call:
  - there is no field with same name in the object;
  - the object is extensible.

**Example**

```cpp
{
  jerry_api_object_t * obj_p = jerry_api_create_object ();

  jerry_api_value_t val;

  ... // initialize val

  // Make new constant field
  jerry_api_add_object_field (obj_p, "some_field_name", &val, false);
}
```


**See also**

- [jerry_api_value_t](#jerryapivaluet)
- [jerry_api_create_object](#jerryapicreateobject)

# jerry_api_delete_object_field

**Summary**
Delete field (property) in the specified object

**Prototype**

```cpp
bool
jerry_api_delete_object_field (jerry_api_object_t * object_p,
                               const char * field_name_p);
```

- `object_p` - object to delete field at;
- `field_name_p` - name of the field.
- returned value - true, if field was deleted successfully, i.e. upon the call:
  - there is field with specified name in the object.

**Example**

```cpp
{
  jerry_api_object_t* obj_p;
  ... // receive or construct obj_p

  jerry_api_delete_object_field (obj_p, "some_field_name");
}
```

**See also**

- [jerry_api_value_t](#jerryapivaluet)
- [jerry_api_create_object](#jerryapicreateobject)

# jerry_api_get_object_field_value

**Summary**
Get value of field (property) in the specified object, i.e. perform [[Get]] operation.

**Prototype**

```cpp
bool
jerry_api_get_object_field_value (jerry_api_object_t * object_p,
                                  const char * field_name_p,
                                  jerry_api_value_t * field_value_p);
```

- `object_p` - object;
- `field_name_p` - name of the field;
- `field_value_p` - retrieved field value (output parameter).
- returned value - true, if field value was retrieved successfully, i.e. upon the call:
  - there is field with specified name in the object.

If value was retrieved successfully, it should be freed with [jerry_api_release_object](#jerryapireleaseobject) just when it becomes unnecessary.

**Example**

```cpp
{
  jerry_api_object_t* obj_p;
  ... // receive or construct obj_p

  jerry_api_value_t val;
  bool is_ok = jerry_api_get_object_field_value (obj_p, "some_field_name", &val);
  if (is_ok)
  {
    ... // usage of 'val'

    jerry_api_release_value (&val);
  }
}
```

**See also**

- [jerry_api_value_t](#jerryapivaluet)
- [jerry_api_create_object](#jerryapicreateobject)

# jerry_api_set_object_field_value

**Summary**
Set value of a field (property) in the specified object, i.e. perform [[Put]] operation.

**Prototype**

```cpp
bool
jerry_api_set_object_field_value (jerry_api_object_t * object_p,
                                  const char * field_name_p,
                                  jerry_api_value_t * field_value_p);
```

- `object_p` - object;
- `field_name_p` - name of the field;
- `field_value_p` - field value to set.
- returned value - true, if field value was set successfully, i.e. upon the call:
  - field value is writable.

**Example**

```cpp
{
  jerry_api_object_t* obj_p;
  jerry_api_value_t val;

  ... // receive or construct obj_p and val

  bool is_ok = jerry_api_set_object_field_value (obj_p, "some_field_name", &val);
}
```

**See also**

- [jerry_api_value_t](#jerryapivaluet)
- [jerry_api_create_object](#jerryapicreateobject)

# jerry_api_get_object_native_handle

**Summary**

Get native handle, previously associated with specified object.

**Prototype**

```cpp
bool
jerry_api_get_object_native_handle (jerry_api_object_t * object_p,
                                    uintptr_t* out_handle_p);
```

- `object_p` - object to get handle from;
- `out_handle_p` - handle value (output parameter);
- returned value - true, if there is handle associated with the object.

**Example**

```cpp
{
  jerry_api_object_t* obj_p;
  uintptr_t handle_set;

  ... // receive or construct obj_p and handle_set value

  jerry_api_set_object_native_handle (obj_p, handle_set, NULL);

  ...

  uintptr_t handle_get;
  bool is_there_associated_handle = jerry_api_get_object_native_handle (obj_p,
                                                                        &handle_get);
}
```

**See also**

- [jerry_api_create_object](#jerryapicreateobject)
- [jerry_api_set_object_native_handle](#jerryapisetobjectnativehandle)

# jerry_api_set_object_native_handle

**Summary**

Set native handle and, optionally, "free" callback for the specified object.

If native handle or "free" callback were already set for the object, corresponding value is updated.

**Prototype**

```cpp
void
jerry_api_set_object_native_handle (jerry_api_object_t * object_p,
                                    uintptr_t handle,
                                    jerry_object_free_callback_t freecb_p);
```

- `object_p` - object to set handle in;
- `handle` - handle value;
- `freecb_p` - pointer to "free" callback or NULL (if not NULL the callback would be called upon GC of the object).

**Example**

```cpp
{
  jerry_api_object_t* obj_p;
  uintptr_t handle_set;

  ... // receive or construct obj_p and handle_set value

  jerry_api_set_object_native_handle (obj_p, handle_set, NULL);

  ...

  uintptr_t handle_get;
  bool is_there_associated_handle = jerry_api_get_object_native_handle (obj_p,
                                                                        &handle_get);
}
```

**See also**

- [jerry_api_create_object](#jerryapicreateobject)
- [jerry_api_get_object_native_handle](#jerryapigetobjectnativehandle)

# jerry_api_is_function

**Summary**
Check whether the specified object is a function object.

**Prototype**

```cpp
bool
jerry_api_is_function (const jerry_api_object_t* object_p);
```

- `object_p` - object to check;
- returned value - just boolean, indicating whether the specified object can be called as function.

**Example**

```cpp
{
  jerry_api_value_t val;

  ... // receiving val

  if (val.type == JERRY_API_DATA_TYPE_OBJECT) {
    if (jerry_api_is_function (val.v_object)) {
      // the object is function object
    }
  }
}
```

**See also**

- [jerry_api_value_t](#jerryapivaluet)
- [jerry_api_is_constructor](#jerryapiisconstructor)
- [jerry_api_call_function](#jerryapicallfunction)

# jerry_api_is_constructor

**Summary**
Check whether the specified object is a constructor function object.

**Prototype**

```cpp
bool
jerry_api_is_constructor (const jerry_api_object_t* object_p);
```

- `object_p` - object to check;
- returned value - just boolean, indicating whether the specified object can be called as constructor.

**Example**

```cpp
{
  jerry_api_value_t val;

  ... // receiving val

  if (val.type == JERRY_API_DATA_TYPE_OBJECT) {
    if (jerry_api_is_constructor (val.v_object)) {
      // the object is constructor function object
    }
  }
}
```

**See also**

- [jerry_api_value_t](#jerryapivaluet)
- [jerry_api_is_function](#jerryapiisfunction)
- [jerry_api_construct_object](#jerryapiconstructobject)

# jerry_api_call_function

**Summary**
Call function object.

**Prototype**

```cpp
bool
jerry_api_call_function (jerry_api_object_t * function_object_p,
                         jerry_api_object_t * this_arg_p,
                         jerry_api_value_t * retval_p,
                         const jerry_api_value_t args_p[],
                         uint16_t args_count);
```

- `function_object_p` - the function object to call;
- `this_arg_p` - object to use as 'this' during the invocation, or NULL - to set the Global object as 'this';
- `retval_p` - function's return value (output parameter);
- `args_p`, `args_count` - array of arguments and number of them;
- returned value - true, if call was performed successfully, i.e.:
  - specified object is a function object (see also jerry_api_is_function);
  - no unhandled exceptions were thrown in connection with the call.

 If call was performed successfully, returned value should be freed with [jerry_api_release_object](#jerryapireleaseobject) just when it becomes unnecessary.

**Example**

```cpp
{
  jerry_api_value_t val;

  ... // receiving val

  if (val.type == JERRY_API_DATA_TYPE_OBJECT) {
    if (jerry_api_is_function (val.v_object)) {
      jerry_api_value_t ret_val;

      bool is_ok = jerry_api_call_function (val.v_object,
                                            NULL,
                                            &ret_val,
                                            NULL, 0);

      if (is_ok) {
        ... // handle return value

        jerry_api_release_value (&ret_val);
      }
    }
  }
}
```

**See also**

- [jerry_api_is_function](#jerryapiisfunction)
- [jerry_api_value_t](#jerryapivaluet)
- [jerry_api_create_external_function](#jerryapicreateexternalfunction)

# jerry_api_construct_object

**Summary**
Construct object invoking specified function object as constructor.

**Prototype**

```cpp
bool
jerry_api_construct_object (jerry_api_object_t * function_object_p,
                            jerry_api_value_t * retval_p,
                            const jerry_api_value_t args_p[],
                            uint16_t args_count);
```

- `function_object_p` - the function object to invoke;
- `retval_p` - return value of function invoked as constructor, i.e. like with 'new' operator (output parameter);
- `args_p`, `args_count` - array of arguments and number of them;
- returned value - true, if call was performed successfully, i.e.:
  - specified object is a constructor function object;
  - no unhandled exceptions were thrown in connection with the call.

If call was performed successfully, returned value should be freed with [jerry_api_release_object](#jerryapireleaseobject) just when it becomes unnecessary.

**Example**

```cpp
{
  jerry_api_value_t val;

  ... // receiving val

  if (val.type == JERRY_API_DATA_TYPE_OBJECT) {
    if (jerry_api_is_constructor (val.v_object)) {
      jerry_api_value_t ret_val;

      bool is_ok = jerry_api_construct_object (val.v_object,
                                               &ret_val,
                                               NULL, 0);

      if (is_ok) {
        ... // handle return value

        jerry_api_release_value (&ret_val);
      }
    }
  }
}
```

**See also**

 - [jerry_api_is_constructor](#jerryapiisconstructor)
 - [jerry_api_value_t](#jerryapivaluet)

# jerry_external_handler_t

**Summary**

The data type represents pointer to call handler of a native function object.

**Structure**

```cpp
typedef bool (* jerry_external_handler_t) (const jerry_api_object_t * function_obj_p,
                                          const jerry_api_value_t * this_p,
                                          jerry_api_value_t * ret_val_p,
                                          const jerry_api_value_t args_p[],
                                          const uint16_t args_count);
```

**See also**

- [jerry_api_create_external_function](#jerryapicreateexternalfunction)

# jerry_api_create_external_function

**Summary**
Create an external function object.

**Prototype**

```cpp
jerry_api_object_t*
jerry_api_create_external_function (jerry_external_handler_t handler_p);
```

- `handler_p` - pointer to native handler of the function object;
- returned value - pointer to constructed external function object.

Received pointer should be released with [jerry_api_release_object](#jerryapireleaseobject), just when the value becomes unnecessary.

**Example**

```cpp
static bool
handler (const jerry_api_object_t * function_obj_p,
         const jerry_api_value_t * this_p,
         jerry_api_value_t * ret_val_p,
         const jerry_api_value_t args_p[],
         const uint16_t args_cnt)
{
  printf ("native handler called!\n");

  ret_val_p->type = JERRY_API_DATA_TYPE_BOOLEAN;
  ret_val_p->v_bool = true;
}

{
  jerry_api_object_t * obj_p = jerry_api_create_external_function (handler);
  jerry_api_object_t * glob_obj_p = jerry_api_get_global ();

  jerry_api_value_t val;
  val.type = JERRY_API_DATA_TYPE_OBJECT;
  val.v_object = obj_p;

  // after this, script can invoke the native handler through "handler_field (1, 2, 3);"
  jerry_api_set_object_field_value (glob_obj_p, "handler_field", &val);

  jerry_api_release_object (glob_obj_p);
  jerry_api_release_object (obj_p);
}
```

**See also**

- [jerry_external_handler_t](#jerryexternalhandlert)
- [jerry_api_is_function](#jerryapiisfunction)
- [jerry_api_call_function](#jerryapicallfunction)
- [jerry_api_release_object](#jerryapireleaseobject)

# jerry_api_create_array_object

**Summary**
Create new JavaScript array object.

Upon the JavaScript array object becomes unused, all pointers to it should be released using [jerry_api_release_object](#jerryapireleaseobject).

**Prototype**

```cpp
jerry_api_object_t*
jerry_api_create_array_object (jerry_api_size_t array_size);
```

 - `array_size` - size of array;
 - returned value is pointer to the created array object.

 **Example**

```cpp
{
    jerry_api_object_t * array_object_p = jerry_api_create_array_object (10);

    ...

    jerry_api_release_object (array_object_p);
}
```

**See also**

- [jerry_api_acquire_object](#jerryapiacquireobject)
- [jerry_api_release_object](#jerryapireleaseobject)
- [jerry_api_set_array_index_value](#jerryapisetarrayindexvalue)
- [jerry_api_get_array_index_value](#jerryapigetarrayindexvalue)
- [jerry_api_add_object_field](#jerryapiaddobjectfield)
- [jerry_api_delete_object_field](#jerryapideleteobjectfield)
- [jerry_api_get_object_field_value](#jerryapigetobjectfieldvalue)
- [jerry_api_set_object_field_value](#jerryapisetobjectfieldvalue)
- [jerry_api_get_object_native_handle](#jerryapigetobjectnativehandle)
- [jerry_api_set_object_native_handle](#jerryapisetobjectnativehandle)

# jerry_api_set_array_index_value

**Summary**
Set value of an indexed element in the specified array object.

**Prototype**

```cpp
bool
jerry_api_set_array_index_value (jerry_api_object_t * array_object_p,
                                 jerry_api_length_t index,
                                 jerry_api_value_t * value_p);
```

- `array_object_p` - pointer to the array object;
- `index` - index of the array element;
- `value_p` - indexed value to set;
- returned value - true, if value was set successfully.

**Example**

```cpp
{
    jerry_api_object_t * array_object_p = jerry_api_create_array_object (10);
    jerry_api_value_t val;

    ... // receive or construct val

    jerry_api_set_array_index_value (array_object_p, 5, &val);

    jerry_api_release_object (array_object_p);
}
```

**See also**

- [jerry_api_value_t](#jerryapivaluet)
- [jerry_api_create_array_object](#jerryapicreatearrayobject)

# jerry_api_get_array_index_value

**Summary**
Get value of an indexed element in the specified array object.

**Prototype**

```cpp
bool
jerry_api_get_array_index_value (jerry_api_object_t * array_object_p,
                                 jerry_api_length_t index,
                                 jerry_api_value_t * value_p);
```

- `array_object_p` - pointer to the array object;
- `index` - index of the array element;
- `value_p` - retrieved indexed value (output parameter);
- returned value - true, if value was retrieved successfully.

**Example**

```cpp
{
    jerry_api_object_t* array_object_p;
    ... // receive or construct array_object_p

    jerry_api_value_t val;
    bool is_ok = jerry_api_get_array_index_value (array_object_p, 5, &val);
    if (is_ok)
    {
        ... // usage of 'val'
    }
}
```

**See also**

- [jerry_api_value_t](#jerryapivaluet)
- [jerry_api_create_array_object](#jerryapicreatearrayobject)

# jerry_api_release_value

**Summary**
Release specified pointer to the value.

**Prototype**

```cpp
void
jerry_api_release_value (jerry_api_value_t * value_p);
```

- `value_p` - pointer to the value.

**Example**
```cpp
{
    jerry_api_value_t val1;
    jerry_api_value_t val2;

    val1.type = JERRY_API_DATA_TYPE_OBJECT;
    val1.v_object = jerry_api_create_object ();

    val2.type = JERRY_API_DATA_TYPE_STRING;
    val2.v_string = jerry_api_create_string ("abc");

    ... // usage of val1

    jerry_api_release_value (&val1);

    ... // usage of val2

    jerry_api_release_value (&val2);
}
```

**See also**

- [jerry_api_value_t](#jerryapivaluet)

# jerry_api_create_error

**Summary**
Create new JavaScript error object.
it should be throwed inside of handle attached to external function object.

**Prototype**

```cpp
jerry_api_object_t*
jerry_api_create_error (jerry_api_error_t error_type,
                        const jerry_api_char_t * message_p);
```

- `error_type` - error type of object;
- `message_p` - human-readable description of the error;
- returned value is pointer to the created error object.

**Example**

```cpp
static bool
handler (const jerry_api_object_t * function_obj_p,
         const jerry_api_value_t * this_p,
         jerry_api_value_t * ret_val_p,
         const jerry_api_value_t args_p[],
         const uint16_t args_cnt)
{
  jerry_api_object_t * error_p = jerry_api_create_error (JERRY_API_ERROR_TYPE,
                                                         (jerry_api_char_t * ) "error");

  jerry_api_acquire_object (error_p);
  ret_val_p->type = JERRY_API_DATA_TYPE_OBJECT;
  ret_val_p->v_object = error_p;

  jerry_api_release_object (error_p);

  return false;
}

{
  jerry_api_object_t * throw_obj_p = jerry_api_create_external_function (handler);
  jerry_api_object_t * glob_obj_p = jerry_api_get_global ();

  jerry_api_value_t val;
  val.type = JERRY_API_DATA_TYPE_OBJECT;
  val.v_object = throw_obj_p;

  // after this, script can invoke the native handler through "error_func ();"
  // and "error_func" throw a error on called
  jerry_api_set_object_field_value (glob_obj_p, "error_func", &val);

  jerry_api_release_object (glob_obj_p);
  jerry_api_release_object (throw_obj_p);
}
```

**See also**
- [jerry_external_handler_t](#jerryexternalhandlert)
- [jerry_api_is_function](#jerryapiisfunction)
- [jerry_api_call_function](#jerryapicallfunction)
- [jerry_api_release_object](#jerryapireleaseobject)
- [jerry_api_create_external_function](#jerryapicreateexternalfunction)
