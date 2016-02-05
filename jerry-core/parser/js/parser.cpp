/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
 * Copyright 2015-2016 University of Szeged.
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

#include "js-parser-internal.h"
#include "parser.h"

/** \addtogroup parser Parser
 * @{
 *
 * \addtogroup jsparser JavaScript
 * @{
 *
 * \addtogroup jsparser_parser Parser
 * @{
 */

/**
 * Parse EcamScript source code
 */
jsp_status_t
parser_parse_script (const jerry_api_char_t *source_p, /**< source code */
                     size_t size, /**< size of the source code */
                     ecma_compiled_code_t **bytecode_data_p) /**< result */
{
  *bytecode_data_p = parser_parse_script (source_p, size, false, NULL);

  if (!*bytecode_data_p)
  {
    return JSP_STATUS_SYNTAX_ERROR;
  }

  return JSP_STATUS_OK;
} /* parser_parse_script */

/**
 * Parse EcamScript eval source code
 */
jsp_status_t
parser_parse_eval (const jerry_api_char_t *source_p, /**< source code */
                   size_t size, /**< size of the source code */
                   bool is_strict, /**< strict mode */
                   ecma_compiled_code_t **bytecode_data_p) /**< result */
{
  *bytecode_data_p = parser_parse_script (source_p, size, is_strict, NULL);

  if (!*bytecode_data_p)
  {
    return JSP_STATUS_SYNTAX_ERROR;
  }

  return JSP_STATUS_OK;
} /* parser_parse_eval */

/**
 * @}
 * @}
 * @}
 */
