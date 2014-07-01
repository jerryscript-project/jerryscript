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

static const char* generated_source = ""
"// AST\n"
"// ECMA has no way of including files. Do we need such feature?\n"
"// EG: Not in initial version\n"
"require (leds);\n"
"\n"
"function LEDsOn () {\n"
"LEDOn (LED3);\n"
"LEDOn (LED6);\n"
"LEDOn (LED7);\n"
"LEDOn (LED4);\n"
"LEDOn (LED10);\n"
"LEDOn (LED8);\n"
"LEDOn (LED9);\n"
"LEDOn (LED5);\n"
"}\n"
"\n"
"function LEDsOff () {\n"
"LEDOff (LED3);\n"
"LEDOff (LED6);\n"
"LEDOff (LED7);\n"
"LEDOff (LED4);\n"
"LEDOff (LED10);\n"
"LEDOff (LED8);\n"
"LEDOff (LED9);\n"
"LEDOff (LED5);\n"
"}\n"
"\n"
"/*\n"
"IMHO we don't need this function in our code,\n"
"we may perform lazy LEDs initialization.\n"
"*/\n"
"// initLEDs ();\n"
"\n"
"while (true) {\n"
"setTimeout (LEDsOn, 500);\n"
"setTimeout (LEDsOff, 500);\n"
"}\n"
;
