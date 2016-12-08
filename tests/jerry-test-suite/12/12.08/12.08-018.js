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

var o = {p1: 1,
  p2: {p1: 100, p2: 200, p3: 100},
  p3: 4,
  p4: 7,
  p5: 124686,
  p6: {p1: 100, p2: 200, p3: 100},
  p7: 1},
sum = 0;

top:
        for (var p in o)
{
  if (p === "p4")
    break;

  if (typeof (o[p]) === "object")
  {
    for (var pp in o[p])
    {
      if (pp === "p2")
        break top;

      sum += o[p][pp];
    }
  }

  sum += 20;

}

assert(sum === 120)
