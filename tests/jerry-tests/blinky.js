// Copyright 2014 Samsung Electronics Co., Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// AST
// ECMA has no way of including files. Do we need such feature?
// EG: Not in initial version
require (leds);

function LEDsOn () {
     LEDOn (LED3);
     LEDOn (LED6);
     LEDOn (LED7);
     LEDOn (LED4);
     LEDOn (LED10);
     LEDOn (LED8);
     LEDOn (LED9);
     LEDOn (LED5);
}

function LEDsOff () {
     LEDOff (LED3);
     LEDOff (LED6);
     LEDOff (LED7);
     LEDOff (LED4);
     LEDOff (LED10);
     LEDOff (LED8);
     LEDOff (LED9);
     LEDOff (LED5);
}

/*
 IMHO we don't need this function in our code,
 we may perform lazy LEDs initialization.
 */
// initLEDs ();

while (true) {
  setTimeout (LEDsOn, 500);
  setTimeout (LEDsOff, 500);
}
