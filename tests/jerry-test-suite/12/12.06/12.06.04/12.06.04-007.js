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

var o = {a: 1, b: 2, c: 3, d: 4, e: 5};

function test()
{
  for (var p in o)
  {
    if (p === 'c')
      return 1;

    o[p] += 4;
  }
  return 0;
}

var r = test();

assert(((o.a === 5 && o.b === 6 && o.c === 3) ||
        (o.c === 3 && o.d === 8 && o.e === 9)) && r === 1);
