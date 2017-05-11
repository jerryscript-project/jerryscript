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

#include "config.h"
#include "jerryscript.h"
#include "test-common.h"

static bool test_context_data1_new_called = false;
static bool test_context_data2_new_called = false;
static bool test_context_data1_free_called = false;
static bool test_context_data2_free_called = false;

typedef struct
{
  jerry_context_data_header_t base;
  const char *string;
} test_context_data_t;

/* Context item 1 */
static test_context_data_t item1;

static jerry_context_data_header_t *
test_context_data1_new (void)
{
  test_context_data1_new_called = true;
  return ((jerry_context_data_header_t *) (&item1));
} /* test_context_data1_new */

static void
test_context_data1_free (jerry_context_data_header_t *item_p)
{
  test_context_data1_free_called = true;
  TEST_ASSERT (((void *) item_p) == ((void *) (&item1)));
} /* test_context_data1_free */

static const jerry_context_data_manager_t manager1 =
{
  .init_cb = test_context_data1_new,
  .deinit_cb = test_context_data1_free
};

static test_context_data_t item1 =
{
  .base = { .manager_p = &manager1, .next_p = NULL },
  .string = "item1"
};

/* Context item 2 */
static test_context_data_t item2;

static jerry_context_data_header_t *
test_context_data2_new (void)
{
  test_context_data2_new_called = true;
  return ((jerry_context_data_header_t *) (&item2));
} /* test_context_data2_new */

static void
test_context_data2_free (jerry_context_data_header_t *item_p)
{
  test_context_data2_free_called = true;
  TEST_ASSERT (((void *) item_p) == ((void *) (&item2)));
} /* test_context_data2_free */

static const jerry_context_data_manager_t manager2 =
{
  .init_cb = test_context_data2_new,
  .deinit_cb = test_context_data2_free
};

static test_context_data_t item2 =
{
  .base = { .manager_p = &manager2, .next_p = NULL },
  .string = "item2"
};

int
main (void)
{
  TEST_INIT ();

  jerry_init (JERRY_INIT_EMPTY);

  TEST_ASSERT (!strcmp (((test_context_data_t *) jerry_get_context_data (&manager1))->string, "item1"));

  TEST_ASSERT (!strcmp (((test_context_data_t *) jerry_get_context_data (&manager2))->string, "item2"));

  TEST_ASSERT (test_context_data1_new_called);
  TEST_ASSERT (test_context_data2_new_called);

  jerry_cleanup ();

  TEST_ASSERT (test_context_data1_free_called);
  TEST_ASSERT (test_context_data2_free_called);

  return 0;
} /* main */
