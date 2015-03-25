/* Copyright 2015 Samsung Electronics Co., Ltd.
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

#ifndef IO_EXTENSION_DESCRIPTION_INC_H
#define IO_EXTENSION_DESCRIPTION_INC_H

#define EXTENSION_NAME "io"
#define EXTENSION_DESCRIPTION_HEADER "io-extension-description.inc.h"

#include "jerry-extension.inc.h"

#endif /* IO_EXTENSION_DESCRIPTION_INC_H */

#if defined (EXTENSION_FUNCTION)
EXTENSION_FUNCTION (print_uint32, plugin_io_print_uint32,
                    1,
                    EXTENSION_ARG (0, UINT32))
EXTENSION_FUNCTION (print_string, plugin_io_print_string,
                    1,
                    EXTENSION_ARG (0, STRING))
#elif defined (EXTENSION_FIELD)
#if defined (__TARGET_HOST)
EXTENSION_FIELD (platform, STRING, "linux")
#elif defined (__TARGET_MCU_STM32F3)
EXTENSION_FIELD (platform, STRING, "mcu_stm32f3")
#elif defined (__TARGET_MCU_STM32F4)
EXTENSION_FIELD (platform, STRING, "mcu_stm32f4")
#endif /* !__TARGET_MCU_STM32F3 && __TARGET_MCU_STM32F4 */
#endif /* !EXTENSION_FUNCTION && !EXTENSION_FIELD */
