// Copyright 2015 Samsung Electronics Co., Ltd.
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

var led_green = 0;
var led_orange = 1;
var led_red = 2;
var led_blue = 3;

var leds = new Array (led_green, led_orange, led_red, led_blue);
var waitDivisors = new Array (1, 2, 3, 6);
var waitTime = 60;
var numOfIterations = 10;

while (1)
{
  for (var j = 0; j < numOfIterations; j += 1)
  {
    for (var led = 0; led < leds.length; led++)
    {
      LEDOn (leds [led]);
    }

    wait (waitTime * 2);

    for (var led = 0; led < leds.length; led++)
    {
      LEDOff (leds [led]);
    }

    wait (waitTime * 2);
  }

  for (var j = 0; j < numOfIterations; j += 1)
  {
    for (var led = 0; led < leds.length; led++)
    {
      LEDOn (leds [led]);
      wait (waitTime);
      LEDOff (leds [led]);
      wait (waitTime);
    }
  }

  for (var d = 0; d < waitDivisors.length; d++)
  {
    var divisor = waitDivisors [d];

    for (var j = 0; j < numOfIterations; j += 1)
    {
      for (var led = 0; led < leds.length; led++)
      {
        LEDOn (leds [led]);
        wait (waitTime / divisor);
        LEDOff (leds [led]);
      }
    }
  }
}
