/* Copyright 2014 Samsung Electronics Co., Ltd.
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

/**
 * Implementation of exit with specified status code.
 */

#include "globals.h"
#include "jerry-libc.h"

/*
 * Exit with specified status code.
 *
 * If !JERRY_NDEBUG and code != 0, print status code with description
 * and call assertion fail handler.
 */
void __noreturn
jerry_exit (jerry_status_t code) /**< status code */
{
#ifndef JERRY_NDEBUG
  if (code != ERR_OK)
  {
    __printf ("Error: ");

    switch (code)
    {
      case ERR_OK:
      {
        JERRY_UNREACHABLE();
        break;
      }
      case ERR_IO:
      {
        __printf ("ERR_IO\n");
        break;
      }
      case ERR_BUFFER_SIZE:
      {
        __printf ("ERR_BUFFER_SIZE\n");
        break;
      }
      case ERR_SEVERAL_FILES:
      {
        __printf ("ERR_SEVERAL_FILES\n");
        break;
      }
      case ERR_NO_FILES:
      {
        __printf ("ERR_NO_FILES\n");
        break;
      }
      case ERR_NON_CHAR:
      {
        __printf ("ERR_NON_CHAR\n");
        break;
      }
      case ERR_UNCLOSED:
      {
        __printf ("ERR_UNCLOSED\n");
        break;
      }
      case ERR_INT_LITERAL:
      {
        __printf ("ERR_INT_LITERAL\n");
        break;
      }
      case ERR_STRING:
      {
        __printf ("ERR_STRING\n");
        break;
      }
      case ERR_PARSER:
      {
        __printf ("ERR_PARSER\n");
        break;
      }
      case ERR_MEMORY:
      {
        __printf ("ERR_MEMORY\n");
        break;
      }
      case ERR_SYSCALL:
      {
        JERRY_UNREACHABLE();
        break;
      }
      case ERR_UNHANDLED_EXCEPTION:
      {
        __printf ("ERR_UNHANDLED_EXCEPTION\n");
        break;
      }
      case ERR_UNIMPLEMENTED_CASE:
      {
        __printf ("ERR_UNIMPLEMENTED_CASE\n");
        break;
      }
      case ERR_FAILED_ASSERTION_IN_SCRIPT:
      {
        __printf ("ERR_FAILED_ASSERTION_IN_SCRIPT\n");
        break;
      }
      case ERR_FAILED_INTERNAL_ASSERTION:
      {
        __printf ("ERR_FAILED_INTERNAL_ASSERTION\n");
        break;
      }
    }
  }
#endif /* !JERRY_NDEBUG */

  __exit (-code);
} /* jerry_exit */

