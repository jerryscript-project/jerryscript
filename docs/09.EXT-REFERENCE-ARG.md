# jerryx_arg types

## jerryx_arg_t

**Summary**

The structure defining a single validation/transformation step.

*Note*: For commonly used validators, `arg.h` provides helpers to create the `jerryx_arg_t`s.
For example, `jerryx_arg_number ()`, `jerryx_arg_boolean ()`, etc.

**Prototype**

```c
typedef struct
{
  /** the transform function */
  jerryx_arg_transform_func_t func;
  /** pointer to destination where func should store the result */
  void *dest;
  /** extra information, specific to func */
  uintptr_t extra_info;
} jerryx_arg_t;
```

**See also**

- [jerryx_arg_number](#jerryx_arg_number)
- [jerryx_arg_boolean](#jerryx_arg_boolean)
- [jerryx_arg_string](#jerryx_arg_string)
- [jerryx_arg_utf8_string](#jerryx_arg_utf8_string)
- [jerryx_arg_function](#jerryx_arg_function)
- [jerryx_arg_native_pointer](#jerryx_arg_native_pointer)
- [jerryx_arg_ignore](#jerryx_arg_ignore)
- [jerryx_arg_object_properties](#jerryx_arg_object_properties)

## jerryx_arg_object_props_t

**Summary**

The structure is used in `jerryx_arg_object_properties`. It provides the properties' names,
its corresponding JS-to-C mapping and other related information.

**Prototype**

```c
typedef struct
{
  const jerry_char_t **name_p; /**< property name list of the JS object */
  jerry_length_t name_cnt; /**< count of the name list */
  const jerryx_arg_t *c_arg_p; /**< points to the array of transformation steps */
  jerry_length_t c_arg_cnt; /**< the count of the `c_arg_p` array */
} jerryx_arg_object_props_t;
```

**See also**

- [jerryx_arg_object_properties](#jerryx_arg_object_properties)

## jerryx_arg_array_items_t

**Summary**

The structure is used in `jerryx_arg_array`. It provides the array items' corresponding
JS-to-C mappings and count.

**Prototype**

```c
typedef struct
{
  const jerryx_arg_t *c_arg_p; /**< points to the array of transformation steps */
  jerry_length_t c_arg_cnt; /**< the count of the `c_arg_p` array */
} jerryx_arg_array_items_t;
```

**See also**

- [jerryx_arg_array](#jerryx_arg_array)

## jerryx_arg_transform_func_t

**Summary**

Signature of the transform function.

Users can create custom transformations by implementing a transform function
and using `jerryx_arg_custom ()`.

The function is expected to return `undefined` if it ran successfully or
return an `Error` in case it failed. The function can use the iterator and the
helpers `jerryx_arg_js_iterator_pop ()` and `jerryx_arg_js_iterator_peek ()` to
get the next input value.

*Note*: A transform function is allowed to consume any number of input values!
This enables complex validation like handling different JS function signatures,
mapping multiple input arguments to a C struct, etc.

The function is expected to store the result of
a successful transformation into `c_arg_p->dest`. In case the validation did
not pass, the transform should not modify `c_arg_p->dest`.

Additional parameters can be provided to the function through `c_arg_p->extra_info`.

**Prototype**

```c
typedef jerry_value_t (*jerryx_arg_transform_func_t) (jerryx_arg_js_iterator_t *js_arg_iter_p,
                                                      const jerryx_arg_t *c_arg_p);
```

**See also**

- [jerryx_arg_custom](#jerryx_arg_custom)
- [jerryx_arg_js_iterator_pop](#jerryx_arg_js_iterator_pop)
- [jerryx_arg_js_iterator_peek](#jerryx_arg_js_iterator_peek)


## jerryx_arg_coerce_t

Enum that indicates whether an argument is allowed to be coerced into the expected JS type.

 - JERRYX_ARG_COERCE - the transform will invoke toNumber, toBoolean, toString, etc.
 - JERRYX_ARG_NO_COERCE - the type coercion is not allowed. The transform will fail if the type does not match the expectation.

**See also**

- [jerryx_arg_number](#jerryx_arg_number)
- [jerryx_arg_boolean](#jerryx_arg_boolean)
- [jerryx_arg_string](#jerryx_arg_string)

## jerryx_arg_optional_t

Enum that indicates whether an argument is optional or required.

 - JERRYX_ARG_OPTIONAL - The argument is optional. If the argument is `undefined` the transform is successful and `c_arg_p->dest` remains untouched.
 - JERRYX_ARG_REQUIRED - The argument is required. If the argument is `undefined` the transform will fail and `c_arg_p->dest` remains untouched.

**See also**

- [jerryx_arg_number](#jerryx_arg_number)
- [jerryx_arg_boolean](#jerryx_arg_boolean)
- [jerryx_arg_string](#jerryx_arg_string)
- [jerryx_arg_function](#jerryx_arg_function)
- [jerryx_arg_native_pointer](#jerryx_arg_native_pointer)

## jerryx_arg_round_t

Enum that indicates the rounding policy which will be chosen to transform an integer.

 - JERRYX_ARG_ROUND - use round() method.
 - JERRYX_ARG_FLOOR - use floor() method.
 - JERRYX_ARG_CEIL - use ceil() method.

**See also**

- [jerryx_arg_uint8](#jerryx_arg_uint8)
- [jerryx_arg_uint16](#jerryx_arg_uint16)
- [jerryx_arg_uint32](#jerryx_arg_uint32)
- [jerryx_arg_int8](#jerryx_arg_int8)
- [jerryx_arg_int16](#jerryx_arg_int16)
- [jerryx_arg_int32](#jerryx_arg_int32)


## jerryx_arg_clamp_t

 Indicates the clamping policy which will be chosen to transform an integer.
 If the policy is NO_CLAMP, and the number is out of range,
 then the transformer will throw a range error.

 - JERRYX_ARG_CLAMP - clamp the number when it is out of range
 - JERRYX_ARG_NO_CLAMP - throw a range error

**See also**

- [jerryx_arg_uint8](#jerryx_arg_uint8)
- [jerryx_arg_uint16](#jerryx_arg_uint16)
- [jerryx_arg_uint32](#jerryx_arg_uint32)
- [jerryx_arg_int8](#jerryx_arg_int8)
- [jerryx_arg_int16](#jerryx_arg_int16)
- [jerryx_arg_int32](#jerryx_arg_int32)

# Main functions

## jerryx_arg_transform_this_and_args

**Summary**

Validate the this value and the JS arguments, and assign them to the native arguments.
This function is useful to perform input validation inside external function handlers (see `jerry_external_handler_t`).

**Prototype**

```c
jerry_value_t
jerryx_arg_transform_this_and_args (const jerry_value_t this_val,
                                    const jerry_value_t *js_arg_p,
                                    const jerry_length_t js_arg_cnt,
                                    const jerryx_arg_t *c_arg_p,
                                    jerry_length_t c_arg_cnt)
```

 - `this_val` - `this` value. Note this is processed as the first value, before the array of arguments.
 - `js_arg_p` - points to the array with JS arguments.
 - `js_arg_cnt` - the count of the `js_arg_p` array.
 - `c_arg_p` - points to the array of validation/transformation steps
 - `c_arg_cnt` - the count of the `c_arg_p` array.
 - return value - a `jerry_value_t` representing `undefined` if all validators passed or an `Error` if a validator failed.

**Example**

[doctest]: # (test="compile")

```c
#include "jerryscript.h"
#include "jerryscript-ext/arg.h"

/* JS signature: function (requiredBool, requiredString, optionalNumber) */
static jerry_value_t
my_external_handler (const jerry_value_t function_obj,
                     const jerry_value_t this_val,
                     const jerry_value_t args_p[],
                     const jerry_length_t args_count)
{
  bool required_bool;
  char required_str[16];
  double optional_num = 1234.567;  // default value

  /* "mapping" defines the steps to transform input arguments to C variables. */
  const jerryx_arg_t mapping[] =
  {
    /* `this` is the first value. No checking needed on `this` for this function. */
    jerryx_arg_ignore (),

    jerryx_arg_boolean (&required_bool, JERRYX_ARG_NO_COERCE, JERRYX_ARG_REQUIRED),
    jerryx_arg_string (required_str, sizeof (required_str), JERRYX_ARG_NO_COERCE, JERRYX_ARG_REQUIRED),
    jerryx_arg_number (&optional_num, JERRYX_ARG_NO_COERCE, JERRYX_ARG_OPTIONAL),
  };

  /* Validate and transform. */
  const jerry_value_t rv = jerryx_arg_transform_this_and_args (this_val,
                                                               args_p,
                                                               args_count,
                                                               mapping,
                                                               4);

  if (jerry_value_is_error (rv))
  {
    /* Handle error. */
    return rv;
  }

  /*
   * Validated and transformed successfully!
   * required_bool, required_str and optional_num can now be used.
   */

  return jerry_create_undefined (); /* Or return something more meaningful. */
}
```

**See also**

- [jerryx_arg_ignore](#jerryx_arg_ignore)
- [jerryx_arg_number](#jerryx_arg_number)
- [jerryx_arg_boolean](#jerryx_arg_boolean)
- [jerryx_arg_string](#jerryx_arg_string)
- [jerryx_arg_function](#jerryx_arg_function)
- [jerryx_arg_native_pointer](#jerryx_arg_native_pointer)
- [jerryx_arg_custom](#jerryx_arg_custom)
- [jerryx_arg_object_properties](#jerryx_arg_object_properties)


## jerryx_arg_transform_args

**Summary**

Validate an array of `jerry_value_t` and assign them to the native arguments.

**Prototype**

```c
jerry_value_t
jerryx_arg_transform_args (const jerry_value_t *js_arg_p,
                           const jerry_length_t js_arg_cnt,
                           const jerryx_arg_t *c_arg_p,
                           jerry_length_t c_arg_cnt)
```

 - `js_arg_p` - points to the array with JS arguments.
 - `js_arg_cnt` - the count of the `js_arg_p` array.
 - `c_arg_p` - points to the array of validation/transformation steps
 - `c_arg_cnt` - the count of the `c_arg_p` array.
 - return value - a `jerry_value_t` representing `undefined` if all validators passed or an `Error` if a validator failed.

**See also**

- [jerryx_arg_transform_this_and_args](#jerryx_arg_transform_this_and_args)


## jerryx_arg_transform_object_properties

**Summary**

Validate the properties of a JS object and assign them to the native arguments.

*Note*: This function transforms properties of a single JS object into native C values.
To transform multiple objects in one pass (for example when converting multiple arguments
to an external handler), please use `jerryx_arg_object_properties` together with
`jerryx_arg_transform_this_and_args` or `jerryx_arg_transform_args`.

**Prototype**

```c
jerry_value_t
jerryx_arg_transform_object_properties (const jerry_value_t obj_val,
                                        const jerry_char_t **name_p,
                                        const jerry_length_t name_cnt,
                                        const jerryx_arg_t *c_arg_p,
                                        jerry_length_t c_arg_cnt);

```

 - `obj_val` - the JS object.
 - `name_p` - points to the array of property names.
 - `name_cnt` - the count of the `name_p` array.
 - `c_arg_p` - points to the array of validation/transformation steps
 - `c_arg_cnt` - the count of the `c_arg_p` array.
 - return value - a `jerry_value_t` representing `undefined` if all validators passed or an `Error` if a validator failed.

**See also**

- [jerryx_arg_object_properties](#jerryx_arg_object_properties)

## jerryx_arg_transform_array

**Summary**

Validate the JS array and assign its items to the native arguments.

*Note*: This function transforms items of a single JS array into native C values.
To transform multiple JS arguments in one pass, please use `jerryx_arg_array` together with
`jerryx_arg_transform_this_and_args` or `jerryx_arg_transform_args`.

**Prototype**

```c
jerry_value_t
jerryx_arg_transform_array (const jerry_value_t array_val,
                            const jerryx_arg_t *c_arg_p,
                            jerry_length_t c_arg_cnt);

```

 - `array_val` - the JS array.
 - `c_arg_p` - points to the array of validation/transformation steps
 - `c_arg_cnt` - the count of the `c_arg_p` array.
 - return value - a `jerry_value_t` representing `undefined` if all validators passed or an `Error` if a validator failed.

**See also**

- [jerryx_arg_array](#jerryx_arg_array)


# Helpers for commonly used validations

## jerryx_arg_uint8

## jerryx_arg_uint16

## jerryx_arg_uint32

## jerryx_arg_int8

## jerryx_arg_int16

## jerryx_arg_int32

**Summary**

All above jerryx_arg_[u]intX functions are used to create a validation/transformation step
(`jerryx_arg_t`) that expects to consume one `number` JS argument
and stores it into a C integer (uint8, int8, uint16, ...)

**Prototype**

Take jerryx_arg_int32 as an example

```c
static inline jerryx_arg_t
jerryx_arg_int32 (int32_t *dest,
                  jerryx_arg_round_t round_flag,
                  jerryx_arg_clamp_t clamp_flag,
                  jerryx_arg_coerce_t coerce_flag,
                  jerryx_arg_optional_t opt_flag);
```

 - return value - the created `jerryx_arg_t` instance.
 - `dest` - pointer to the `int32_t` where the result should be stored.
 - `round_flag` - the rounding policy.
 - `clamp_flag` - the clamping policy.
 - `coerce_flag` - whether type coercion is allowed.
 - `opt_flag` - whether the argument is optional.

**See also**

- [jerryx_arg_transform_this_and_args](#jerryx_arg_transform_this_and_args)


## jerryx_arg_number

**Summary**

Create a validation/transformation step (`jerryx_arg_t`) that expects to consume
one `number` JS argument and stores it into a C `double`.

**Prototype**

```c
static inline jerryx_arg_t
jerryx_arg_number (double *dest,
                   jerryx_arg_coerce_t coerce_flag,
                   jerryx_arg_optional_t opt_flag)
```

 - return value - the created `jerryx_arg_t` instance.
 - `dest` - pointer to the `double` where the result should be stored.
 - `coerce_flag` - whether type coercion is allowed.
 - `opt_flag` - whether the argument is optional.

**See also**

- [jerryx_arg_transform_this_and_args](#jerryx_arg_transform_this_and_args)

## jerryx_arg_boolean

**Summary**

Create a validation/transformation step (`jerryx_arg_t`) that expects to
consume one `boolean` JS argument and stores it into a C `bool`.

**Prototype**

```c
static inline jerryx_arg_t
jerryx_arg_boolean (bool *dest,
                    jerryx_arg_coerce_t coerce_flag,
                    jerryx_arg_optional_t opt_flag)
```
 - return value - the created `jerryx_arg_t` instance.
 - `dest` - pointer to the `bool` where the result should be stored.
 - `coerce_flag` - whether type coercion is allowed.
 - `opt_flag` - whether the argument is optional.

**See also**

- [jerryx_arg_transform_this_and_args](#jerryx_arg_transform_this_and_args)


## jerryx_arg_string

**Summary**

Create a validation/transformation step (`jerryx_arg_t`) that expects to
consume one `string` JS argument and stores it into a CESU-8 C `char` array.

**Prototype**

```c
static inline jerryx_arg_t
jerryx_arg_string (char *dest,
                   uint32_t size,
                   jerryx_arg_coerce_t coerce_flag,
                   jerryx_arg_optional_t opt_flag)
```

 - return value - the created `jerryx_arg_t` instance.
 - `dest` - pointer to the native char array where the result should be stored.
 - `size` - the size of native char array.
 - `coerce_flag` - whether type coercion is allowed.
 - `opt_flag` - whether the argument is optional.

**See also**

- [jerryx_arg_transform_this_and_args](#jerryx_arg_transform_this_and_args)
- [jerry_arg_utf8_string](#jerry_arg_utf8_string)


## jerryx_arg_utf8_string

**Summary**

Create a validation/transformation step (`jerryx_arg_t`) that expects to
consume one `string` JS argument and stores it into a UTF-8 C `char` array.

**Prototype**

```c
static inline jerryx_arg_t
jerryx_arg_utf8_string (char *dest,
                        uint32_t size,
                        jerryx_arg_coerce_t coerce_flag,
                        jerryx_arg_optional_t opt_flag)
```

 - return value - the created `jerryx_arg_t` instance.
 - `dest` - pointer to the native char array where the result should be stored.
 - `size` - the size of native char array.
 - `coerce_flag` - whether type coercion is allowed.
 - `opt_flag` - whether the argument is optional.

**See also**

- [jerryx_arg_transform_this_and_args](#jerryx_arg_transform_this_and_args)
- [jerry_arg_string](#jerry_arg_string)


## jerryx_arg_function

**Summary**

Create a validation/transformation step (`jerryx_arg_t`) that expects to
consume one `function` JS argument and stores it into a C `jerry_value_t`.

**Prototype**

```c
static inline jerryx_arg_t
jerryx_arg_function (jerry_value_t *dest,
                     jerryx_arg_optional_t opt_flag)

```
 - return value - the created `jerryx_arg_t` instance.
 - `dest` - pointer to the `jerry_value_t` where the result should be stored.
 - `opt_flag` - whether the argument is optional.

**See also**

- [jerryx_arg_transform_this_and_args](#jerryx_arg_transform_this_and_args)

## jerryx_arg_native_pointer

**Summary**

Create a validation/transformation step (`jerryx_arg_t`) that expects to
consume one `object` JS argument that is 'backed' with a native pointer with
a given type info. In case the native pointer info matches, the transform
will succeed and the object's native pointer will be assigned to `*dest`.

**Prototype**

```c
static inline jerryx_arg_t
jerryx_arg_native_pointer (void **dest,
                           const jerry_object_native_info_t *info_p,
                           jerryx_arg_optional_t opt_flag)
```
 - return value - the created `jerryx_arg_t` instance.
 - `dest` - pointer to where the resulting native pointer should be stored.
 - `info_p` - expected the type info.
 - `opt_flag` - whether the argument is optional.

**See also**

- [jerryx_arg_transform_this_and_args](#jerryx_arg_transform_this_and_args)

## jerryx_arg_object_properties

**Summary**

Create a validation/transformation step (`jerryx_arg_t`) that expects to
consume one `object` JS argument and call `jerryx_arg_transform_object_properties` inside
to transform its properties to native arguments.
User should prepare the `jerryx_arg_object_props_t` instance, and pass it to this function.

**Prototype**

```c
static inline jerryx_arg_t
jerryx_arg_object_properties (const jerryx_arg_object_props_t *object_props_p,
                              jerryx_arg_optional_t opt_flag);
```
 - return value - the created `jerryx_arg_t` instance.
 - `object_props_p` - provides information for properties transform.
 - `opt_flag` - whether the argument is optional.

**Example**

[doctest]: # (test="compile")

```c
#include "jerryscript.h"
#include "jerryscript-ext/arg.h"

/**
 * The binding function expects args_p[0] is an object, which has 3 properties:
 *     "enable": boolean
 *     "data": number
 *     "extra_data": number, optional
 */
static jerry_value_t
my_external_handler (const jerry_value_t function_obj,
                     const jerry_value_t this_val,
                     const jerry_value_t args_p[],
                     const jerry_length_t args_count)
{
  bool required_bool;
  double required_num;
  double optional_num = 1234.567;  // default value

  /* "prop_name_p" defines the name list of the expected properties' names. */
  const char *prop_name_p[] = { "enable", "data", "extra_data" };

  /* "prop_mapping" defines the steps to transform properties to C variables. */
  const jerryx_arg_t prop_mapping[] =
  {
    jerryx_arg_boolean (&required_bool, JERRYX_ARG_COERCE, JERRYX_ARG_REQUIRED),
    jerryx_arg_number (&required_num, JERRYX_ARG_COERCE, JERRYX_ARG_REQUIRED),
    jerryx_arg_number (&optional_num, JERRYX_ARG_COERCE, JERRYX_ARG_OPTIONAL)
  };

  /* Prepare the jerryx_arg_object_props_t instance. */
  const jerryx_arg_object_props_t prop_info =
  {
    .name_p = (const jerry_char_t **) prop_name_p,
    .name_cnt = 3,
    .c_arg_p = prop_mapping,
    .c_arg_cnt = 3
  };

  /* It is the mapping used in the jerryx_arg_transform_args. */
  const jerryx_arg_t mapping[] =
  {
    jerryx_arg_object_properties (&prop_info, JERRYX_ARG_REQUIRED)
  };

  /* Validate and transform. */
  const jerry_value_t rv = jerryx_arg_transform_args (args_p,
                                                      args_count,
                                                      mapping,
                                                      1);

  if (jerry_value_is_error (rv))
  {
    /* Handle error. */
    return rv;
  }

  /*
   * Validated and transformed successfully!
   * required_bool, required_num and optional_num can now be used.
   */

   return jerry_create_undefined (); /* Or return something more meaningful. */
}

```

 **See also**

- [jerryx_arg_transform_this_and_args](#jerryx_arg_transform_this_and_args)
- [jerryx_arg_transform_object_properties](#jerryx_arg_transform_object_properties)

## jerryx_arg_array

**Summary**

Create a validation/transformation step (`jerryx_arg_t`) that expects to
consume one `array` JS argument and call `jerryx_arg_transform_array_items` inside
to transform its items to native arguments.
User should prepare the `jerryx_arg_array_items_t` instance, and pass it to this function.

**Prototype**

```c
static inline jerryx_arg_t
jerryx_arg_array (const jerryx_arg_array_items_t *array_items_p, jerryx_arg_optional_t opt_flag);
```
 - return value - the created `jerryx_arg_t` instance.
 - `array_items_p` - provides items information for transform.
 - `opt_flag` - whether the argument is optional.

**Example**

[doctest]: # (test="compile")

```c
#include "jerryscript.h"
#include "jerryscript-ext/arg.h"

/**
 * The binding function expects args_p[0] is an array, which has 3 items:
 *     first: boolean
 *     second: number
 *     third: number, optional
 */
static jerry_value_t
my_external_handler (const jerry_value_t function_obj,
                     const jerry_value_t this_val,
                     const jerry_value_t args_p[],
                     const jerry_length_t args_count)
{
  bool required_bool;
  double required_num;
  double optional_num = 1234.567;  // default value

  /* "item_mapping" defines the steps to transform array items to C variables. */
  const jerryx_arg_t item_mapping[] =
  {
    jerryx_arg_boolean (&required_bool, JERRYX_ARG_COERCE, JERRYX_ARG_REQUIRED),
    jerryx_arg_number (&required_num, JERRYX_ARG_COERCE, JERRYX_ARG_REQUIRED),
    jerryx_arg_number (&optional_num, JERRYX_ARG_COERCE, JERRYX_ARG_OPTIONAL)
  };

  /* Prepare the jerryx_arg_array_items_t instance. */
  const jerryx_arg_array_items_t array_info =
  {
    .c_arg_p = item_mapping,
    .c_arg_cnt = 3
  };

  /* It is the mapping used in the jerryx_arg_transform_args. */
  const jerryx_arg_t mapping[] =
  {
    jerryx_arg_array (&array_info, JERRYX_ARG_REQUIRED)
  };

  /* Validate and transform. */
  const jerry_value_t rv = jerryx_arg_transform_args (args_p,
                                                      args_count,
                                                      mapping,
                                                      1);

  if (jerry_value_is_error (rv))
  {
    /* Handle error. */
    return rv;
  }

  /*
   * Validated and transformed successfully!
   * required_bool, required_num and optional_num can now be used.
   */

   return jerry_create_undefined (); /* Or return something more meaningful. */
}

```

 **See also**

- [jerryx_arg_transform_this_and_args](#jerryx_arg_transform_this_and_args)
- [jerryx_arg_transform_object_properties](#jerryx_arg_transform_object_properties)

# Functions to create custom validations

## jerryx_arg_custom

**Summary**

Create a jerryx_arg_t instance with custom transform.

**Prototype**

```c
static inline jerryx_arg_t
jerryx_arg_custom (void *dest,
                   uintptr_t extra_info,
                   jerryx_arg_transform_func_t func)

```
 - return value - the created `jerryx_arg_t` instance.
 - `dest` - pointer to the native argument where the result should be stored.
 - `extra_info` - the extra parameter data, specific to the transform function.
 - `func` - the custom transform function.

**See also**

- [jerryx_arg_transform_this_and_args](#jerryx_arg_transform_this_and_args)



## jerryx_arg_js_iterator_pop

**Summary**

Pop the current `jerry_value_t` argument from the iterator.
It will change the `js_arg_idx` and `js_arg_p` value in the iterator.

**Prototype**

```c
jerry_value_t
jerryx_arg_js_iterator_pop (jerryx_arg_js_iterator_t *js_arg_iter_p)
```
 - return value - the `jerry_value_t` argument that was popped.
 - `js_arg_iter_p` - the JS arg iterator from which to pop.

## jerryx_arg_js_iterator_peek

**Summary**

Get the current JS argument from the iterator, without moving the iterator forward.
*Note:* Unlike `jerryx_arg_js_iterator_pop ()`, it will not change `js_arg_idx` and
`js_arg_p` value in the iterator.

**Prototype**

```c
jerry_value_t
jerryx_arg_js_iterator_peek (jerryx_arg_js_iterator_t *js_arg_iter_p)
```
 - return value - the current `jerry_value_t` argument.
 - `js_arg_iter_p` - the JS arg iterator from which to peek.

## jerryx_arg_js_iterator_restore

**Summary**

Restore the last item popped from the stack.  This can be called as
many times as there are arguments on the stack -- if called when the
first element in the array is the current top of the stack, this
function does nothing.

*Note:* This function relies on the underlying implementation of the
arg stack as an array, as its function is to simply back up the "top
of stack" pointer to point to the previous element of the array.

*Note:* Like `jerryx_arg_js_iterator_pop ()`, this function will
change the `js_arg_idx` and `js_arg_p` values in the iterator.

**Prototype**

```c
jerry_value_t
jerryx_arg_js_iterator_restore (jerryx_arg_js_iterator_t *js_arg_iter_p)
```
 - return value - the the new top of the stack.
 - `js_arg_iter_p` - the JS arg iterator to restore.


## jerryx_arg_js_iterator_index

**Summary**

Get the index of the current JS argument from the iterator.

**Prototype**

```c
jerry_length_t
jerryx_arg_js_iterator_index (jerryx_arg_js_iterator_t *js_arg_iter_p)
```
 - return value - the index of current JS argument.
 - `js_arg_iter_p` - the JS arg iterator from which to peek.
