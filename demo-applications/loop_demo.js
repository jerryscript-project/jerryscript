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
    LEDOn(12);
    LEDOn(13);
    LEDOn(14);
    LEDOn(15);
    wait(waitTime * 2);
    LEDOff(12);
    LEDOff(13);
    LEDOff(14);
    LEDOff(15);
    wait(waitTime * 2);
  }

  for (var j = 0; j < numOfIterations; j += 1)
  {
    LEDOn(12);
    wait(waitTime);
    LEDOff(12);
    wait(waitTime);
    LEDOn(13);
    wait(waitTime);
    LEDOff(13);
    wait(waitTime);
    LEDOn(14);
    wait(waitTime);
    LEDOff(14);
    wait(waitTime);
    LEDOn(15);
    wait(waitTime);
    LEDOff(15);
    wait(waitTime);
  }

  for (var j = 0; j < numOfIterations; j += 1)
  {
    LEDOn(12);
    wait(waitTime);
    LEDOff(12);
    LEDOn(13);
    wait(waitTime);
    LEDOff(13);
    LEDOn(14);
    wait(waitTime);
    LEDOff(14);
    LEDOn(15);
    wait(waitTime);
    LEDOff(15);
  }

  for (var j = 0; j < numOfIterations; j += 1)
  {
    LEDOn(12);
    wait(waitTime / 2);
    LEDOff(12);
    LEDOn(13);
    wait(waitTime / 2);
    LEDOff(13);
    LEDOn(14);
    wait(waitTime / 2);
    LEDOff(14);
    LEDOn(15);
    wait(waitTime / 2);
    LEDOff(15);
  }

  for (var j = 0; j < numOfIterations; j += 1)
  {
    LEDOn(12);
    wait(waitTime / 3);
    LEDOff(12);
    LEDOn(13);
    wait(waitTime / 3);
    LEDOff(13);
    LEDOn(14);
    wait(waitTime / 3);
    LEDOff(14);
    LEDOn(15);
    wait(waitTime / 3);
    LEDOff(15);
  }

  for (var j = 0; j < numOfIterations; j += 1)
  {
    LEDOn(12);
    wait(waitTime / 6);
    LEDOff(12);
    LEDOn(13);
    wait(waitTime / 6);
    LEDOff(13);
    LEDOn(14);
    wait(waitTime / 6);
    LEDOff(14);
    LEDOn(15);
    wait(waitTime / 6);
    LEDOff(15);
  }
}
