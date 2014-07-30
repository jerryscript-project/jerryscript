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
"var count = 10000;\n"
"var x = 7;\n"
"var y = 3;\n"
"\n"
"var tmp1;\n"
"var tmp2;\n"
"var tmp3;\n"
"var tmp4;\n"
"\n"
"for (var i = 0; i < count; i++)\n"
"{\n"
"tmp1 = x * x;\n"
"tmp2 = y * y;\n"
"tmp3 = tmp1 * tmp1;\n"
"tmp4 = tmp2 * tmp2;\n"
"}\n"
;
