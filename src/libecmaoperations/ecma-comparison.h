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

#ifndef JERRY_ECMA_COMPARISON_H
#define JERRY_ECMA_COMPARISON_H

#include "ecma-globals.h"
#include "ecma-helpers.h"

/** \addtogroup ecma ---TODO---
 * @{
 * 
 * \addtogroup ecmacomparison ECMA comparison
 * @{
 */

extern bool ecma_abstract_equality_compare( ecma_value_t x, ecma_value_t y);
extern ecma_completion_value_t ecma_abstract_relational_compare(ecma_value_t x, ecma_value_t y, bool left_first);

/**
 * @}
 * @}
 */

#endif /* !JERRY_ECMA_COMPARISON_H */
