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

#include "globals.h"

static const char* generated_source = ""
        "var tmp, a, b = 1, c = 2, d, e = 3, g = 4;\n"
        "var count = 10000*10000;\n"
        "for (var i = 0; i < count; i += 1)\n"
        "{\n"
        "  tmp = b * c;\n"
        "  a = tmp + g;\n"
        "  d = tmp * e;\n"
        "\n"
        "  if (i % 1000 == 0) \n"
        "  { \n"
        "    LEDOn (12);\n"
        "    LEDOn (13);\n"
        "    LEDOn (14);\n"
        "    LEDOn (15);\n"
        "    wait (300000);\n"
        "    LEDOff (12);\n"
        "    LEDOff (13);\n"
        "    LEDOff (14);\n"
        "    LEDOff (15);\n"
        "  }\n"
        "}\n"
        ;
