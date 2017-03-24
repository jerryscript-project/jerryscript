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
#include "jerry-port.h"
#include "jerry-port-default.h"
#include "jmem.h"
#include "jrt.h"

#ifndef CONFIG_DISABLE_ES2015_PROMISE_BUILTIN

typedef struct jerry_port_queueitem_t jerry_port_queueitem_t;

/**
 * Description of the queue item.
 */
struct jerry_port_queueitem_t
{
  jerry_port_queueitem_t *next_p; /**< points to next item */
  jerry_job_handler_t handler; /**< the handler for the job*/
  void *job_p; /**< points to the job */
};

/**
 * Description of a job queue (FIFO)
 */
typedef struct
{
  jerry_port_queueitem_t *head_p; /**< points to the head item of the queue */
  jerry_port_queueitem_t *tail_p; /**< points to the tail item of the queue*/
} jerry_port_jobqueue_t;

static jerry_port_jobqueue_t queue;

/**
 * Initialize the job queue
 */
void jerry_port_jobqueue_init (void)
{
  queue.head_p = NULL;
  queue.tail_p = NULL;
} /* jerry_port_jobqueue_init */

/**
 * Enqueue a job
 */
void jerry_port_jobqueue_enqueue (jerry_job_handler_t handler, /**< the handler for the job */
                                  void *job_p) /**< the job */
{
  jerry_port_queueitem_t *item_p = jmem_heap_alloc_block (sizeof (jerry_port_queueitem_t));
  item_p->job_p = job_p;
  item_p->handler = handler;

  if (queue.head_p == NULL)
  {
    JERRY_ASSERT (queue.tail_p == NULL);

    queue.head_p = item_p;
    item_p->next_p = NULL;
    queue.tail_p = item_p;

    return;
  }

  JERRY_ASSERT (queue.tail_p != NULL);

  queue.tail_p->next_p = item_p;
  queue.tail_p = item_p;
} /* jerry_port_jobqueue_enqueue */

/**
 * Dequeue and get the job.
 * @return pointer to jerry_port_queueitem_t
 *         It should be freed with jmem_heap_free_block
 */
static jerry_port_queueitem_t *
jerry_port_jobqueue_dequeue (void)
{
  if (queue.head_p == NULL)
  {
    JERRY_ASSERT (queue.tail_p == NULL);

    return NULL;
  }

  JERRY_ASSERT (queue.tail_p != NULL);

  jerry_port_queueitem_t *item_p = queue.head_p;
  queue.head_p = queue.head_p->next_p;

  return item_p;
} /* jerry_port_jobqueue_dequeue */

/**
 * Start the jobqueue.
 * @return jerry value
 *         If exception happens in the handler, stop the queue
 *         and return the exception.
 *         Otherwise, return undefined
 */
jerry_value_t
jerry_port_jobqueue_run (void)
{
  jerry_value_t ret;

  while (true)
  {
    jerry_port_queueitem_t *item_p = jerry_port_jobqueue_dequeue ();

    if (item_p == NULL)
    {
      return jerry_create_undefined ();
    }

    void *job_p = item_p->job_p;
    jerry_job_handler_t handler = item_p->handler;
    jmem_heap_free_block (item_p, sizeof (jerry_port_queueitem_t));
    ret = handler (job_p);

    if (jerry_value_has_error_flag (ret))
    {
      return ret;
    }

    jerry_release_value (ret);
  }
} /* jerry_port_jobqueue_run */

#endif /* !CONFIG_DISABLE_ES2015_PROMISE_BUILTIN */
