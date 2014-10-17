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

var led_green = 0;
var led_orange = 1;
var led_red = 2;
var led_blue = 3;

function LEDSet(led_group, on)
{
  if (on)
  {
    LEDOn (led_group);
    LEDOn (led_group + 4);
  }
  else
  {
    LEDOff (led_group);
    LEDOff (led_group + 4);
  }
}

var tmp, a, b = 1, c = 2, d, e = 3, g = 4;

var count = 1000;

tmp = b * c;
a = tmp + g;
d = tmp * e + a;

var waitTime = 60;
var numOfIterations = 10;

while (1)
{
  for (var j = 0; j < numOfIterations; j += 1)
  {
    LEDSet(led_green, true);
    LEDSet(led_orange, true);
    LEDSet(led_red, true);
    LEDSet(led_blue, true);
    wait(waitTime * 2);
    LEDSet(led_green, false);
    LEDSet(led_orange, false);
    LEDSet(led_red, false);
    LEDSet(led_blue, false);
    wait(waitTime * 2);
  }

  for (var j = 0; j < numOfIterations; j += 1)
  {
    LEDSet(led_green, true);
    wait(waitTime);
    LEDSet(led_green, false);
    wait(waitTime);
    LEDSet(led_orange, true);
    wait(waitTime);
    LEDSet(led_orange, false);
    wait(waitTime);
    LEDSet(led_red, true);
    wait(waitTime);
    LEDSet(led_red, false);
    wait(waitTime);
    LEDSet(led_blue, true);
    wait(waitTime);
    LEDSet(led_blue, false);
    wait(waitTime);
  }

  for (var j = 0; j < numOfIterations; j += 1)
  {
    LEDSet(led_green, true);
    wait(waitTime);
    LEDSet(led_green, false);
    LEDSet(led_orange, true);
    wait(waitTime);
    LEDSet(led_orange, false);
    LEDSet(led_red, true);
    wait(waitTime);
    LEDSet(led_red, false);
    LEDSet(led_blue, true);
    wait(waitTime);
    LEDSet(led_blue, false);
  }

  for (var j = 0; j < numOfIterations; j += 1)
  {
    LEDSet(led_green, true);
    wait(waitTime / 2);
    LEDSet(led_green, false);
    LEDSet(led_orange, true);
    wait(waitTime / 2);
    LEDSet(led_orange, false);
    LEDSet(led_red, true);
    wait(waitTime / 2);
    LEDSet(led_red, false);
    LEDSet(led_blue, true);
    wait(waitTime / 2);
    LEDSet(led_blue, false);
  }

  for (var j = 0; j < numOfIterations; j += 1)
  {
    LEDSet(led_green, true);
    wait(waitTime / 3);
    LEDSet(led_green, false);
    LEDSet(led_orange, true);
    wait(waitTime / 3);
    LEDSet(led_orange, false);
    LEDSet(led_red, true);
    wait(waitTime / 3);
    LEDSet(led_red, false);
    LEDSet(led_blue, true);
    wait(waitTime / 3);
    LEDSet(led_blue, false);
  }

  for (var j = 0; j < numOfIterations; j += 1)
  {
    LEDSet(led_green, true);
    wait(waitTime / 6);
    LEDSet(led_green, false);
    LEDSet(led_orange, true);
    wait(waitTime / 6);
    LEDSet(led_orange, false);
    LEDSet(led_red, true);
    wait(waitTime / 6);
    LEDSet(led_red, false);
    LEDSet(led_blue, true);
    wait(waitTime / 6);
    LEDSet(led_blue, false);
  }
}
