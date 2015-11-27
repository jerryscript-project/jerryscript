JerryScript Engine can be embedded into any application, providing the way to run JavaScript in a large range of environments - from desktops to low-memory microcontrollers.

This guide is intended to introduce you to JerryScript embedding API through creation of simple JavaScript shell.

## Step 1. Execute JavaScript from your application

```cpp
#include <string.h>
#include "jerry.h"

int
main (int argc, char * argv[]) {
  char script [] = "print ('Hello, World!');";

  jerry_completion_code_t code = jerry_run_simple (script,
                                                   strlen (script),
                                                   JERRY_FLAG_EMPTY);
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


```cpp
#include <string.h>
#include "jerry.h"

int
main (int argc, char * argv[]) {
  char script [] = "print ('Hello, World!');";

  // Initialize engine
  jerry_init (JERRY_FLAG_EMPTY);

  // Setup Global scope code
  jerry_parse (script, strlen (script));

  // Execute Global scope code
  jerry_completion_code_t code = jerry_run ();

  // Cleanup engine
  jerry_cleanup ();
}
```

Our code is more complex now, but it introduces possibilities to interact with JerryScript step-by-step: setup native objects, call JavaScript functions, etc.

## Step 3. Execution in 'eval'-mode

```cpp
#include <string.h>
#include "jerry.h"

int
main (int argc, char * argv[]) {
  char script1 [] = "var s = 'Hello, World!';";
  char script2 [] = "print (s);";

  // Initialize engine
  jerry_init (JERRY_FLAG_EMPTY);

  jerry_api_value_t eval_ret;

  // Evaluate script1
  jerry_api_eval (script1, strlen (script1),
                  false, false, &eval_ret);
  // Free JavaScript value, returned by eval
  jerry_api_release_value (&eval_ret);

  // Evaluate script2
  jerry_api_eval (script2, strlen (script2),
                  false, false, &eval_ret);
  // Free JavaScript value, returned by eval
  jerry_api_release_value (&eval_ret);

  // Cleanup engine
  jerry_cleanup ();
}
```

This way, we execute two independent script parts in one execution environment. The first part initializes string variable, and the second outputs the variable.

## Step 4. Interaction with JavaScript environment

```cpp
#include <string.h>
#include "jerry.h"

int
main (int argc, char * argv[]) {
  char str [] = "Hello, World!";
  char var_name [] = "s";
  char script [] = "print (s);";

  // Initializing JavaScript environment
  jerry_init (JERRY_FLAG_EMPTY);

  // Getting pointer to the Global object
  jerry_api_object_t *obj_p = jerry_api_get_global_object ();

  // Constructing string
  jerry_api_string_t *str_val_p = jerry_api_create_string (str);

  // Constructing string value descriptor
  jerry_api_value_t val;
  val.type = JERRY_API_DATA_TYPE_STRING;
  val.string_p = str_val_p;

  // Setting the string value to field of the Global object
  jerry_api_set_object_field_value (obj_p, var_name, &val);

  // Releasing string value, as it is no longer necessary outside of engine
  jerry_api_release_string (str_val_p);

  // Same for pointer to the Global object
  jerry_api_release_object (obj_p);

  jerry_api_value_t eval_ret;

  // Now starting script that would output value of just initialized field
  jerry_api_eval (script, strlen (script),
                  false, false, &eval_ret);
  jerry_api_release_value (&eval_ret);

  // Freeing engine
  jerry_cleanup ();
}
```

The sample will also output 'Hello, World!'. However, now it is not just a part of the source script, but the value, dynamically supplied to the engine.

## Step 5. Description of JavaScript value descriptors

Structure, used to put values to or receive values from the engine is the following:

- `type` of the value:
  - JERRY_API_DATA_TYPE_UNDEFINED (undefined);
  - JERRY_API_DATA_TYPE_NULL (null);
  - JERRY_API_DATA_TYPE_BOOLEAN (true / false);
  - JERRY_API_DATA_TYPE_FLOAT64 (number);
  - JERRY_API_DATA_TYPE_STRING (string);
  - JERRY_API_DATA_TYPE_OBJECT (object reference);
- `v_bool` (if JERRY_API_DATA_TYPE_BOOLEAN) - boolean value;
- `v_float64` (if JERRY_API_DATA_TYPE_FLOAT64) - number value;
- `v_string` (if JERRY_API_DATA_TYPE_STRING) - pointer to string;
- `v_object` (if JERRY_API_DATA_TYPE_OBJECT) - pointer to object.

Abstract values, to be sent to or received from the engine are described with the structure.

Pointers to strings or objects and values should be released just when become unnecessary, using `jerry_api_release_string` or `jerry_api_release_object` and `jerry_api_release_value`, correspondingly.

The following example function will output a JavaScript value:

```cpp
static void
print_value (const jerry_api_value_t * value_p)
{
  switch (value_p->type)
  {
    // Simple values: undefined, null, false, true
    case JERRY_API_DATA_TYPE_UNDEFINED:
      printf ("undefined");
      break;
    case JERRY_API_DATA_TYPE_NULL:
      printf ("null");
      break;
    case JERRY_API_DATA_TYPE_BOOLEAN:
      if (value_p->v_bool)
        printf ("true");
      else
        printf ("false");
      break;

    // Number value
    case JERRY_API_DATA_TYPE_FLOAT64:
      printf ("%lf", value_p->v_float64);
      break;

    // String value
    case JERRY_API_DATA_TYPE_STRING:
    {
      ssize_t neg_req_sz, sz;
      // determining required buffer size
      neg_req_sz = jerry_api_string_to_char_buffer (value_p->v_string,
                                                    NULL,
                                                    0);
      assert (neg_req_sz < 0);
      char * str_buf_p = (char*) malloc (-neg_req_sz);
      sz = jerry_api_string_to_char_buffer (value_p->v_string,
                                            str_buf_p,
                                            -neg_req_sz);
      assert (sz == -neg_req_sz);

      printf ("%s", str_buf_p);

      free (str_buf_p);
      break;
    }

    // Object reference
    case JERRY_API_DATA_TYPE_OBJECT:
      printf ("[JS object]");
      break;
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

```cpp
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jerry.h"

static void
print_value (const jerry_api_value_t * value_p);

int
main (int argc, char * argv[]) {
  // Initialize engine
  jerry_init (JERRY_FLAG_EMPTY);

  char cmd [256];
  while (true) {
    printf ("> ");

    // Input next command
    if (fgets (cmd, sizeof (cmd), stdin) == NULL
        || strcmp (cmd, "quit\n") == 0) {
      // If the command is 'quit', exit from loop
      break;
    }

    jerry_api_value_t ret_val;

    // Evaluate entered command
    jerry_completion_code_t status = jerry_api_eval (cmd, strlen (cmd),
                                                     false, false,
                                                     &ret_val);

    // If command evaluated successfully, print value, returned by eval
    if (status == JERRY_COMPLETION_CODE_OK) {
      // 'eval' completed successfully
      print_value (&ret_val);
      jerry_api_release_value (&ret_val);
    } else {
      // evaluated JS code thrown an exception
      // and didn't handle it with try-catch-finally
      printf ("Unhandled JS exception occured\n");
    }

    printf ("\n");
    fflush (stdout);
  }

  // Cleanup engine
  jerry_cleanup ();

  return 0;
}
```

The application inputs commands and evaluates them, one after another.


## Further steps

For further API description, please look at [Embedding API](/API).
