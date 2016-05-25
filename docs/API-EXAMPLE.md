JerryScript Engine can be embedded into any application, providing the way to run JavaScript in a large range of environments - from desktops to low-memory microcontrollers.

This guide is intended to introduce you to JerryScript embedding API through creation of simple JavaScript shell.

## Step 1. Execute JavaScript from your application

```c
#include <string.h>
#include "jerry.h"

int
main (int argc, char * argv[])
{
  const jerry_api_char_t script[] = "print ('Hello, World!');";
  size_t script_size = strlen ((const char *) script);

  jerry_completion_code_t return_code = jerry_run_simple (script,
                                                          script_size,
                                                          JERRY_FLAG_EMPTY);

  return (int) return_code;
}
```

The application will generate the following output:

```bash
Hello, World!
```

## Step 2. Split engine initialization and script execution

Here we perform the same actions, as `jerry_run_simple`, while splitting into several steps:

- engine initialization
- script code setup
- script execution
- engine cleanup


```c
#include <string.h>
#include "jerry.h"

int
main (int argc, char * argv[])
{
  const jerry_api_char_t script[] = "print ('Hello, World!');";
  size_t script_size = strlen ((const char *) script);

  /* Initialize engine */
  jerry_init (JERRY_FLAG_EMPTY);

  /* Setup Global scope code */
  jerry_api_object_t *error_object_p = NULL;
  if (!jerry_parse (script, script_size, &error_object_p))
  {
    /* Error object must be freed, if parsing failed */
    jerry_api_release_object (error_object_p);
  }
  else
  {
    /* Execute Global scope code */
    jerry_api_value_t error_value = jerry_api_create_void_value ();
    jerry_completion_code_t return_code = jerry_run (&error_value);

    if (return_code == JERRY_COMPLETION_CODE_UNHANDLED_EXCEPTION)
    {
      /* Error value must be freed, if 'jerry_run' returns with an unhandled exception */
      jerry_api_release_value (&error_value);
    }
  }

  /* Cleanup engine */
  jerry_cleanup ();

  return 0;
}
```

Our code is more complex now, but it introduces possibilities to interact with JerryScript step-by-step: setup native objects, call JavaScript functions, etc.

## Step 3. Execution in 'eval'-mode

```c
#include <string.h>
#include "jerry.h"

int
main (int argc, char * argv[])
{
  const jerry_api_char_t script_1[] = "var s = 'Hello, World!';";
  const jerry_api_char_t script_2[] = "print (s);";

  /* Initialize engine */
  jerry_init (JERRY_FLAG_EMPTY);

  jerry_api_value_t eval_ret;

  /* Evaluate script1 */
  jerry_api_eval (script_1,
                  strlen ((const char *) script_1),
                  false,
                  false,
                  &eval_ret);

  /* Free JavaScript value, returned by eval */
  jerry_api_release_value (&eval_ret);

  /* Evaluate script2 */
  jerry_api_eval (script_2,
                  strlen ((const char *) script_2),
                  false,
                  false,
                  &eval_ret);

  /* Free JavaScript value, returned by eval */
  jerry_api_release_value (&eval_ret);

  /* Cleanup engine */
  jerry_cleanup ();

  return 0;
}
```

This way, we execute two independent script parts in one execution environment. The first part initializes string variable, and the second outputs the variable.

## Step 4. Interaction with JavaScript environment

```c
#include <string.h>
#include "jerry.h"

int
main (int argc, char * argv[]) {
  const jerry_api_char_t str[] = "Hello, World!";
  const jerry_api_char_t var_name[] = "s";
  const jerry_api_char_t script[] = "print (s);";

  /* Initializing JavaScript environment */
  jerry_init (JERRY_FLAG_EMPTY);

  /* Getting pointer to the Global object */
  jerry_api_object_t *object_p = jerry_api_get_global ();

  /* Constructing string */
  jerry_api_string_t *str_val_p = jerry_api_create_string (str);

  /* Constructing string value descriptor */
  jerry_api_value_t value;
  value.type = JERRY_API_DATA_TYPE_STRING;
  value.u.v_string = str_val_p;

  /* Setting the string value to field of the Global object */
  jerry_api_set_object_field_value (object_p, var_name, &value);

  /* Releasing string value, as it is no longer necessary outside of engine */
  jerry_api_release_string (str_val_p);

  /* Same for pointer to the Global object */
  jerry_api_release_object (object_p);

  jerry_api_value_t eval_ret;

  /* Now starting script that would output value of just initialized field */
  jerry_api_eval (script,
                  strlen ((const char *) script),
                  false,
                  false,
                  &eval_ret);

  /* Free JavaScript value, returned by eval */
  jerry_api_release_value (&eval_ret);

  /* Freeing engine */
  jerry_cleanup ();

  return 0;
}
```

The sample will also output 'Hello, World!'. However, now it is not just a part of the source script, but the value, dynamically supplied to the engine.

## Step 5. Description of JavaScript value descriptors

Structure, used to put values to or receive values from the engine is the following:

- `type` of the value:
  - JERRY_API_DATA_TYPE_VOID (void);
  - JERRY_API_DATA_TYPE_UNDEFINED (undefined);
  - JERRY_API_DATA_TYPE_NULL (null);
  - JERRY_API_DATA_TYPE_BOOLEAN (true / false);
  - JERRY_API_DATA_TYPE_FLOAT32 (number);
  - JERRY_API_DATA_TYPE_FLOAT64 (number);
  - JERRY_API_DATA_TYPE_UINT32 (number);
  - JERRY_API_DATA_TYPE_STRING (string);
  - JERRY_API_DATA_TYPE_OBJECT (object reference);
- `v_bool` (if JERRY_API_DATA_TYPE_BOOLEAN) - boolean value;
- `v_float32` (if JERRY_API_DATA_TYPE_FLOAT32) - float value;
- `v_float64` (if JERRY_API_DATA_TYPE_FLOAT64) - double value;
- `v_uint32` (if JERRY_API_DATA_TYPE_UINT32) - 32 bit unsigned integer value;
- `v_string` (if JERRY_API_DATA_TYPE_STRING) - pointer to string;
- `v_object` (if JERRY_API_DATA_TYPE_OBJECT) - pointer to object.

Abstract values, to be sent to or received from the engine are described with the structure.

Pointers to strings or objects and values should be released just when become unnecessary, using `jerry_api_release_string` or `jerry_api_release_object` and `jerry_api_release_value`, correspondingly.

The following example function will output a JavaScript value:

```c
#include <stdlib.h>

static void
print_value (const jerry_api_value_t *value_p)
{
  switch (value_p->type)
  {
    /* Simple values: void, undefined, null, false, true */
    case JERRY_API_DATA_TYPE_VOID:
    {
      printf ("void");
      break;
    }
    case JERRY_API_DATA_TYPE_UNDEFINED:
    {
      printf ("undefined");
      break;
    }
    case JERRY_API_DATA_TYPE_NULL:
    {
      printf ("null");
      break;
    }
    case JERRY_API_DATA_TYPE_BOOLEAN:
    {
      if (value_p->u.v_bool)
      {
        printf ("true");
      }
      else
      {
        printf ("false");
      }
      break;
    }
    /* Float value */
    case JERRY_API_DATA_TYPE_FLOAT32:
    {
      printf ("%f", value_p->u.v_float32);
      break;
    }
    /* Double value */
    case JERRY_API_DATA_TYPE_FLOAT64:
    {
      printf ("%lf", value_p->u.v_float64);
      break;
    }
    /* Unsigned integer value */
    case JERRY_API_DATA_TYPE_UINT32:
    {
      printf ("%d", value_p->u.v_uint32);
      break;
    }
    /* String value */
    case JERRY_API_DATA_TYPE_STRING:
    {
      /* Determining required buffer size */
      jerry_api_size_t req_sz = jerry_api_get_string_size (value_p->u.v_string);
      jerry_api_char_t *str_buf_p = (jerry_api_char_t *) malloc (req_sz);
      jerry_api_string_to_char_buffer (value_p->u.v_string,
                                       str_buf_p,
                                       req_sz);

      printf ("%s", (const char *) str_buf_p);

      free (str_buf_p);
      break;
    }
    /* Object reference */
    case JERRY_API_DATA_TYPE_OBJECT:
    {
      printf ("[JS object]");
      break;
    }
  }

  printf ("\n");
}
```

## Simple JavaScript shell

Now all building blocks, necessary to construct JavaScript shell, are ready.

Shell operation can be described with the following loop:

- read command;
- if command is 'quit'
  - exit loop;
- else
  - eval (command);
  - print result of eval;
  - loop.

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jerry.h"

static void print_value (const jerry_api_value_t *value_p);

int
main (int argc, char * argv[])
{
  jerry_completion_code_t status = JERRY_COMPLETION_CODE_OK;

  /* Initialize engine */
  jerry_init (JERRY_FLAG_EMPTY);

  char cmd [256];
  while (true)
  {
    printf ("> ");

    /* Input next command */
    if (fgets (cmd, sizeof (cmd), stdin) == NULL
        || strcmp (cmd, "quit\n") == 0)
    {
      /* If the command is 'quit', exit from loop */
      break;
    }

    jerry_api_value_t ret_val;

    /* Evaluate entered command */
    status = jerry_api_eval ((const jerry_api_char_t *) cmd,
                             strlen (cmd),
                             false,
                             false,
                             &ret_val);

    /* If command evaluated successfully, print value, returned by eval */
    if (status == JERRY_COMPLETION_CODE_OK)
    {
      /* 'eval' completed successfully */
      print_value (&ret_val);
      jerry_api_release_value (&ret_val);
    }
    else
    {
      /* Evaluated JS code thrown an exception
         and didn't handle it with try-catch-finally */
      printf ("Unhandled JS exception occured\n");
    }

    fflush (stdout);
  }

  /* Cleanup engine */
  jerry_cleanup ();

  return (int) status;
}
```

The application inputs commands and evaluates them, one after another.

## Step 6. Creating JS object in global context

In this example we demonstrate how to use native function and structures in JavaScript.

```c
#include <string.h>
#include "jerry.h"

struct my_struct
{
  const char *msg;
} my_struct;

/**
 * Get a string from a native object
 */
static bool
get_msg_handler (const jerry_api_object_t *function_obj_p, /**< function object */
                 const jerry_api_value_t *this_p, /**< this arg */
                 jerry_api_value_t *ret_val_p, /**< return argument */
                 const jerry_api_value_t *args_p, /**< function arguments */
                 const jerry_api_length_t args_cnt) /**< number of function arguments */
{
  jerry_api_string_t *msg_str_p = jerry_api_create_string ((const jerry_api_char_t *) my_struct.msg);
  *ret_val_p = jerry_api_create_string_value (msg_str_p);

  return true;
} /* get_msg_handler */

int
main (int argc, char * argv[])
{
  jerry_completion_code_t status = JERRY_COMPLETION_CODE_OK;

  /* Initialize engine */
  jerry_init (JERRY_FLAG_EMPTY);

  /* Do something with the native object */
  my_struct.msg = "Hello World";

  /* Create an empty JS object */
  jerry_api_object_t *object_p = jerry_api_create_object ();

  /* Create a JS function object and wrap into a jerry value */
  jerry_api_value_t object_value;
  object_value.type = JERRY_API_DATA_TYPE_OBJECT;
  object_value.u.v_object = jerry_api_create_external_function (get_msg_handler);

  /* Set the native function as a property of the empty JS object */
  jerry_api_set_object_field_value (object_p,
                                    (const jerry_api_char_t *) "myFunc",
                                    &object_value);
  jerry_api_release_value (&object_value);

  /* Wrap the JS object (not empty anymore) into a jerry api value */
  object_value.type = JERRY_API_DATA_TYPE_OBJECT;
  object_value.u.v_object = object_p;
  jerry_api_object_t *global_obj_p = jerry_api_get_global ();

  /* Add the JS object to the global context */
  jerry_api_set_object_field_value (global_obj_p,
                                    (const jerry_api_char_t *) "MyObject",
                                    &object_value);
  jerry_api_release_value (&object_value);
  jerry_api_release_object (global_obj_p);

  /* Now we have a "builtin" object called MyObject with a function called myFunc()
   *
   * Equivalent JS code:
   *                    var MyObject = { myFunc : function () { return "some string value"; } }
   */
  const jerry_api_char_t script[] = " \
    var str = MyObject.myFunc (); \
    print (str); \
  ";
  size_t script_size = strlen ((const char *) script);

  jerry_api_value_t eval_ret;

  /* Evaluate script */
  status = jerry_api_eval (script,
                           script_size,
                           false,
                           false,
                           &eval_ret);

  /* Free JavaScript value, returned by eval */
  jerry_api_release_value (&eval_ret);

  /* Cleanup engine */
  jerry_cleanup ();

  return (int) status;
}
```

The application will generate the following output:

```bash
Hello World
```

## Step 7. Extending JS Objects with native functions

Here we create a JS Object with `jerry_api_eval`, then extend it with a native function. This function shows how to get a property value from the object and how to manipulate it.

```c
#include <string.h>
#include "jerry.h"

/**
 * Add param to 'this.x'
 */
static bool
add_handler (const jerry_api_object_t *function_obj_p, /**< function object */
             const jerry_api_value_t *this_p, /**< this arg */
             jerry_api_value_t *ret_val_p, /**< return argument */
             const jerry_api_value_t *args_p, /**< function arguments */
             const jerry_api_length_t args_cnt) /**< number of function arguments */
{
  jerry_api_value_t x_val;

  /* Get 'this.x' */
  if (jerry_api_get_object_field_value (jerry_api_get_object_value (this_p),
                                        (const jerry_api_char_t *) "x",
                                        &x_val))
  {
    /* Convert Jerry API values to double */
    double x = jerry_api_get_number_value (&x_val);
    double d = jerry_api_get_number_value (args_p);

    /* Add the parameter to 'x' */
    jerry_api_value_t res_val = jerry_api_create_number_value (x + d);

    /* Set the new value of 'this.x' */
    jerry_api_set_object_field_value (jerry_api_get_object_value (this_p),
                                      (const jerry_api_char_t *) "x",
                                      &res_val);

  }

  return true;
} /* add_handler */

int
main (int argc, char * argv[])
{
  jerry_completion_code_t status = JERRY_COMPLETION_CODE_OK;

  /* Initialize engine */
  jerry_init (JERRY_FLAG_EMPTY);

  /* Create a JS object */
  const jerry_api_char_t my_js_object[] = " \
    MyObject = \
    { x : 12, \
      y : 'Value of x is ', \
      foo: function () \
      { \
        return this.y + this.x; \
      } \
    } \
  ";

  jerry_api_value_t my_js_obj_val;

  /* Evaluate script */
  status = jerry_api_eval (my_js_object,
                           strlen ((const char *) my_js_object),
                           false,
                           false,
                           &my_js_obj_val);

  jerry_api_object_t *object_p = jerry_api_get_object_value (&my_js_obj_val);

  /* Create a JS function object and wrap into a jerry value */
  jerry_api_value_t object_value;
  object_value.type = JERRY_API_DATA_TYPE_OBJECT;
  object_value.u.v_object = jerry_api_create_external_function (add_handler);

  /* Set the native function as a property of previously created MyObject */
  jerry_api_set_object_field_value (object_p,
                                    (const jerry_api_char_t *) "add2x",
                                    &object_value);
  jerry_api_release_value (&object_value);

  /* Free JavaScript value, returned by eval (my_js_object) */
  jerry_api_release_value (&my_js_obj_val);

  const jerry_api_char_t script[] = " \
    var str = MyObject.foo (); \
    print (str); \
    MyObject.add2x (5); \
    print (MyObject.foo ()); \
  ";
  size_t script_size = strlen ((const char *) script);

  jerry_api_value_t eval_ret;

  /* Evaluate script */
  status = jerry_api_eval (script,
                           script_size,
                           false,
                           false,
                           &eval_ret);

  /* Free JavaScript value, returned by eval */
  jerry_api_release_value (&eval_ret);

  /* Cleanup engine */
  jerry_cleanup ();

  return (int) status;
}
```

The application will generate the following output:

```bash
Value of x is 12
Value of x is 17
```

## Further steps

For further API description, please visit [API Reference page](https://samsung.github.io/jerryscript/API/) on [JerryScript home page](https://samsung.github.io/jerryscript/).
