// Copyright JS Foundation and other contributors, http://js.foundation
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

function test(max, mod)
{
  var array = [];

  for (var i = 0; i < max + 1; i++)
    array[i] = i;

  for (var i = 0; i < max; i++)
    if ((i % mod) == 0)
      delete array[i];

  array.length = array.length - 1;

  for (var i = 0; i < max; i++)
    if ((i % mod) == 0)
      assert(!(i in array));
    else
      assert(array[i] == i);
}

for (var i = 10; i < 16; i++)
  test(i, 2);

for (var i = 10; i < 16; i++)
  test(i, 3);

for (var i = 100; i < 116; i++)
  test(i, 4);

for (var i = 100; i < 116; i++)
  test(i, 5);
