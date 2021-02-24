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

#include "jerryscript.h"

#include "test-common.h"

/* Note: RS = ReSolve, RJ = ReJect */
typedef enum
{
  C = JERRY_PROMISE_EVENT_CREATE, /**< same as JERRY_PROMISE_CALLBACK_CREATE with undefined value */
  RS = JERRY_PROMISE_EVENT_RESOLVE, /**< same as JERRY_PROMISE_CALLBACK_RESOLVE */
  RJ = JERRY_PROMISE_EVENT_REJECT, /**< same as JERRY_PROMISE_CALLBACK_REJECT */
  RSF = JERRY_PROMISE_EVENT_RESOLVE_FULFILLED, /**< same as JERRY_PROMISE_EVENT_RESOLVE_FULFILLED */
  RJF = JERRY_PROMISE_EVENT_REJECT_FULFILLED, /**< same as JERRY_PROMISE_EVENT_REJECT_FULFILLED */
  RWH = JERRY_PROMISE_EVENT_REJECT_WITHOUT_HANDLER, /**< same as JERRY_PROMISE_EVENT_REJECT_WITHOUT_HANDLER */
  CHA = JERRY_PROMISE_EVENT_CATCH_HANDLER_ADDED, /**< same as JERRY_PROMISE_EVENT_CATCH_HANDLER_ADDED */
  BR = JERRY_PROMISE_EVENT_BEFORE_REACTION_JOB, /**< same as JERRY_PROMISE_CALLBACK_BEFORE_REACTION_JOB */
  AR = JERRY_PROMISE_EVENT_AFTER_REACTION_JOB, /**< same as JERRY_PROMISE_CALLBACK_AFTER_REACTION_JOB */
  A = JERRY_PROMISE_EVENT_ASYNC_AWAIT, /**< same as JERRY_PROMISE_CALLBACK_ASYNC_AWAIT */
  BRS = JERRY_PROMISE_EVENT_ASYNC_BEFORE_RESOLVE, /**< same as JERRY_PROMISE_CALLBACK_ASYNC_BEFORE_RESOLVE */
  BRJ = JERRY_PROMISE_EVENT_ASYNC_BEFORE_REJECT, /**< same as JERRY_PROMISE_CALLBACK_ASYNC_BEFORE_REJECT */
  ARS = JERRY_PROMISE_EVENT_ASYNC_AFTER_RESOLVE, /**< same as JERRY_PROMISE_CALLBACK_ASYNC_AFTER_RESOLVE */
  ARJ = JERRY_PROMISE_EVENT_ASYNC_AFTER_REJECT, /**< same as JERRY_PROMISE_CALLBACK_ASYNC_AFTER_REJECT */
  CP = UINT8_MAX - 1, /**< same as JERRY_PROMISE_CALLBACK_CREATE with Promise value */
  E = UINT8_MAX, /**< marks the end of the event list */
} jerry_promise_callback_event_abbreviations_t;

static int user;
static const uint8_t *next_event_p;

static void
promise_callback (jerry_promise_event_type_t event_type, /**< event type */
                  const jerry_value_t object, /**< target object */
                  const jerry_value_t value, /**< optional argument */
                  void *user_p) /**< user pointer passed to the callback */
{
  TEST_ASSERT (user_p == (void *) &user);

  switch (event_type)
  {
    case JERRY_PROMISE_EVENT_CREATE:
    {
      TEST_ASSERT (jerry_value_is_promise (object));
      if (jerry_value_is_undefined (value))
      {
        break;
      }

      TEST_ASSERT (jerry_value_is_promise (value));
      TEST_ASSERT (*next_event_p++ == CP);
      return;
    }
    case JERRY_PROMISE_EVENT_RESOLVE:
    case JERRY_PROMISE_EVENT_REJECT:
    case JERRY_PROMISE_EVENT_RESOLVE_FULFILLED:
    case JERRY_PROMISE_EVENT_REJECT_FULFILLED:
    case JERRY_PROMISE_EVENT_REJECT_WITHOUT_HANDLER:
    {
      TEST_ASSERT (jerry_value_is_promise (object));
      break;
    }
    case JERRY_PROMISE_EVENT_CATCH_HANDLER_ADDED:
    case JERRY_PROMISE_EVENT_BEFORE_REACTION_JOB:
    case JERRY_PROMISE_EVENT_AFTER_REACTION_JOB:
    {
      TEST_ASSERT (jerry_value_is_promise (object));
      TEST_ASSERT (jerry_value_is_undefined (value));
      break;
    }
    case JERRY_PROMISE_EVENT_ASYNC_AWAIT:
    {
      TEST_ASSERT (jerry_value_is_object (object));
      TEST_ASSERT (jerry_value_is_promise (value));
      break;
    }
    default:
    {
      TEST_ASSERT (event_type == JERRY_PROMISE_EVENT_ASYNC_BEFORE_RESOLVE
                   || event_type == JERRY_PROMISE_EVENT_ASYNC_BEFORE_REJECT
                   || event_type == JERRY_PROMISE_EVENT_ASYNC_AFTER_RESOLVE
                   || event_type == JERRY_PROMISE_EVENT_ASYNC_AFTER_REJECT);
      TEST_ASSERT (jerry_value_is_object (object));
      break;
    }
  }

  TEST_ASSERT (*next_event_p++ == (uint8_t) event_type);
} /* promise_callback */

static void
run_eval (const uint8_t *event_list_p, /**< event list */
          const char *source_p) /**< source code */
{
  next_event_p = event_list_p;

  jerry_value_t result = jerry_eval ((const jerry_char_t *) source_p, strlen (source_p), 0);

  TEST_ASSERT (!jerry_value_is_error (result));
  jerry_release_value (result);

  result = jerry_run_all_enqueued_jobs ();
  TEST_ASSERT (!jerry_value_is_error (result));
  jerry_release_value (result);

  TEST_ASSERT (*next_event_p == UINT8_MAX);
} /* run_eval */

int
main (void)
{
  TEST_INIT ();

  if (!jerry_is_feature_enabled (JERRY_FEATURE_PROMISE))
  {
    jerry_port_log (JERRY_LOG_LEVEL_ERROR, "Promise is disabled!\n");
    return 0;
  }

  /* The test system enables this feature when Promises are enabled. */
  TEST_ASSERT (jerry_is_feature_enabled (JERRY_FEATURE_PROMISE_CALLBACK));

  jerry_init (JERRY_INIT_EMPTY);

  jerry_promise_event_filter_t filters = (JERRY_PROMISE_EVENT_FILTER_MAIN
                                          | JERRY_PROMISE_EVENT_FILTER_ERROR
                                          | JERRY_PROMISE_EVENT_FILTER_REACTION_JOB
                                          | JERRY_PROMISE_EVENT_FILTER_ASYNC_MAIN
                                          | JERRY_PROMISE_EVENT_FILTER_ASYNC_REACTION_JOB);

  jerry_promise_set_callback (filters, promise_callback, (void *) &user);

  /* Test promise creation. */
  static uint8_t events1[] = { C, C, C, E };

  run_eval (events1,
            "'use strict'\n"
            "new Promise((res, rej) => {})\n"
            "new Promise((res, rej) => {})\n"
            "new Promise((res, rej) => {})\n");

  /* Test then call. */
  static uint8_t events2[] = { C, CP, E };

  run_eval (events2,
            "'use strict'\n"
            "var promise = new Promise((res, rej) => {})\n"
            "promise.then(() => {}, () => {})\n");

  /* Test then call with extended Promise. */
  static uint8_t events3[] = { C, C, E };

  run_eval (events3,
            "'use strict'\n"
            "var P = class extends Promise {}\n"
            "var promise = new P((res, rej) => {})\n"
            "promise.then(() => {})\n");

  /* Test resolve and reject calls. */
  static uint8_t events4[] = { C, C, RS, RJ, RWH, E };

  run_eval (events4,
            "'use strict'\n"
            "var resolve\n"
            "var reject\n"
            "new Promise((res, rej) => resolve = res)\n"
            "new Promise((res, rej) => reject = rej)\n"
            "resolve(1)\n"
            "reject(1)\n");

  /* Test then and resolve calls. */
  static uint8_t events5[] = { C, CP, RS, BR, RS, AR, E };

  run_eval (events5,
            "'use strict'\n"
            "var resolve\n"
            "var promise = new Promise((res, rej) => resolve = res)\n"
            "promise.then(() => {})\n"
            "resolve(1)\n");

  /* Test resolve and then calls. */
  static uint8_t events6[] = { C, RS, CP, BR, RS, AR, E };

  run_eval (events6,
            "'use strict'\n"
            "var promise = new Promise((res, rej) => res(1))\n"
            "promise.then(() => {})\n");

  /* Test Promise.resolve. */
  static uint8_t events7[] = { C, RS, CP, BR, RS, AR, E };

  run_eval (events7,
            "Promise.resolve(4).then(() => {})\n");

  /* Test Promise.reject. */
  static uint8_t events8[] = { C, RJ, RWH, CP, CHA, BR, RJ, RWH, AR, E };

  run_eval (events8,
            "Promise.reject(4).catch(() => { throw 'Error' })\n");

  /* Test Promise.race without resolve */
  static uint8_t events9[] = { C, C, C, CP, CP, E };

  run_eval (events9,
            "'use strict'\n"
            "var p1 = new Promise((res, rej) => {})\n"
            "var p2 = new Promise((res, rej) => {})\n"
            "Promise.race([p1,p2])\n");

  /* Test Promise.race with resolve. */
  static uint8_t events10[] = { C, RS, C, RJ, RWH, C, CP, CP, CHA, BR, RS, RS, AR, BR, RJF, RS, AR, E };

  run_eval (events10,
            "'use strict'\n"
            "var p1 = new Promise((res, rej) => res(1))\n"
            "var p2 = new Promise((res, rej) => rej(1))\n"
            "Promise.race([p1,p2])\n");

  /* Test Promise.all without resolve. */
  static uint8_t events11[] = { C, C, C, CP, CP, E };

  run_eval (events11,
            "'use strict'\n"
            "var p1 = new Promise((res, rej) => {})\n"
            "var p2 = new Promise((res, rej) => {})\n"
            "Promise.all([p1,p2])\n");

  /* Test Promise.all with resolve. */
  static uint8_t events12[] = { C, RS, C, RJ, RWH, C, CP, CP, CHA, BR, RS, AR, BR, RJ, RWH, RS, AR, E };

  run_eval (events12,
            "'use strict'\n"
            "var p1 = new Promise((res, rej) => res(1))\n"
            "var p2 = new Promise((res, rej) => rej(1))\n"
            "Promise.all([p1,p2])\n");

  /* Test async function. */
  static uint8_t events13[] = { C, RS, E };

  run_eval (events13,
            "'use strict'\n"
            "async function f() {}\n"
            "f()\n");

  /* Test await with resolved Promise. */
  static uint8_t events14[] = { C, RS, A, C, BRS, RS, ARS, E };

  run_eval (events14,
            "'use strict'\n"
            "async function f(p) { await p }\n"
            "f(Promise.resolve(1))\n");

  /* Test await with non-Promise value. */
  static uint8_t events15[] = { C, RS, A, C, BRS, C, RS, A, ARS, BRS, RS, ARS, E };

  run_eval (events15,
            "'use strict'\n"
            "async function f(p) { await p; await 'X' }\n"
            "f(Promise.resolve(1))\n");

  /* Test await with rejected Promise. */
  static uint8_t events16[] = { C, RJ, RWH, A, CHA, C, BRJ, C, RS, RS, ARJ, E };

  run_eval (events16,
            "'use strict'\n"
            "async function f(p) { try { await p; } catch (e) { Promise.resolve(1) } }\n"
            "f(Promise.reject(1))\n");

  /* Test async generator function. */
  static uint8_t events17[] = { C, RS, C, A, BRS, RS, ARS, E };

  run_eval (events17,
            "'use strict'\n"
            "async function *f(p) { await p; return 4 }\n"
            "f(Promise.resolve(1)).next()\n");

  /* Test yield* operation. */
  static uint8_t events18[] = { C, C, RS, A, BRS, C, RS, A, ARS, BRS, RS, ARS, E };

  run_eval (events18,
            "'use strict'\n"
            "async function *f(p) { yield 1 }\n"
            "async function *g() { yield* f() }\n"
            "g().next()\n");

  /* Test multiple fulfill operations. */
  static uint8_t events19[] = { C, RS, RSF, RJF, E };

  run_eval (events19,
            "'use strict'\n"
            "var resolve, reject\n"
            "var p1 = new Promise((res, rej) => { resolve = res, reject = rej })\n"
            "resolve(1)\n"
            "resolve(2)\n"
            "reject(3)\n");

  /* Test multiple fulfill operations. */
  static uint8_t events20[] = { C, RJ, RWH, RSF, RJF, E };

  run_eval (events20,
            "'use strict'\n"
            "var resolve, reject\n"
            "var p1 = new Promise((res, rej) => { resolve = res, reject = rej })\n"
            "reject(1)\n"
            "resolve(2)\n"
            "reject(3)\n");

  /* Test catch handler added later is reported only once. */
  static uint8_t events21[] = { C, RJ, RWH, CP, CHA, CP, CP, BR, RS, AR, BR, RS, AR, BR, RS, AR, E };

  run_eval (events21,
            "'use strict'\n"
            "var rej = Promise.reject(4)\n"
            "rej.catch(() => {})\n"
            "rej.catch(() => {})\n"
            "rej.catch(() => {})\n");

  /* Test catch handler added later is reported only once. */
  static uint8_t events22[] = { C, RJ, RWH, A, CHA, C, BRJ, A, ARJ, BRJ, RJ, RWH, ARJ, E };

  run_eval (events22,
            "'use strict'\n"
            "async function f(p) { try { await p; } catch(e) { await p; } }"
            "f(Promise.reject(4))\n");

  /* Test chained then. */
  static uint8_t events23[] = { C, RJ, RWH, CP, CHA, CP, BR, RJ, AR, BR, RS, AR, E };

  run_eval (events23,
            "'use strict'\n"
            "var p = Promise.reject(0)\n"
            "p.then(() => {}).catch(() => {})\n");

  /* Test disabled filters. */
  jerry_promise_set_callback (JERRY_PROMISE_EVENT_FILTER_DISABLE, promise_callback, (void *) &user);

  static uint8_t events24[] = { E };

  run_eval (events24,
            "'use strict'\n"
            "async function f(p) { await p }"
            "f(Promise.resolve(1))\n");

  /* Test filtered events. */
  filters = JERRY_PROMISE_EVENT_FILTER_REACTION_JOB | JERRY_PROMISE_EVENT_FILTER_ASYNC_REACTION_JOB;
  jerry_promise_set_callback (filters, promise_callback, (void *) &user);

  static uint8_t events25[] = { BR, AR, BRS, ARS, E };

  run_eval (events25,
            "'use strict'\n"
            "async function f(p) { await p }"
            "f(Promise.resolve(1).then(() => {}))\n");

  jerry_cleanup ();
  return 0;
} /* main */
