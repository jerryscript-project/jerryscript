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

var arr = [];
for (let i = 0, j = 0; i < 10; j++)
{
  arr[i] = function() { return i + j; }
  i++;
}

for (let i = 0; i < 10; i++)
  assert(arr[i]() === (i * 2 + 1));

var j = 0, k = 0;
for (let i = 0; j = i, i < 10; i++)
{
  let i = -3;
  assert(i === -3);
  assert(j === k);
  k++;
}

var j = 0, k = 0;
for (let i = 0; eval("j = i"), i < 10; i++)
{
  let i = -3;
  assert(i === -3);
  assert(j === k);
  k++;
}

var arr = [];
for (let i in { x:1, y:1, z:1 })
{
  let str = "P";
  arr.push(function () { return str + i; });
}

assert(arr[0]() === "Px");
assert(arr[1]() === "Py");
assert(arr[2]() === "Pz");

try {
  for (let i in (function() { return i; })()) {}
  assert(false);
} catch (e) {
  assert(e instanceof ReferenceError);
}

try {
  for (let i = 0, j = 0; i < 5; i++, j++)
  {
    if (i === 3)
    {
      eval("throw -42")
    }
  }
  assert(false);
} catch (e) {
  assert(e === -42);
}

exit: {
  for (let i = 0, j = 0; i < 5; i++, j++)
  {
    if (eval("i === 3")) {
      assert(i === 3);
      break exit;
    }
  }
  assert(false);
}

var f = null, g = null, h = null;

for (let i = 0;
     f = function() { return i }, i < 1;
     i++, g = function() { return i })
{
  h = function() { return i };
}
assert(f() === 1);
assert(g() === 1);
assert(h() === 0);

var arr = [];
for (const i in { aa:4, bb:5, cc:6 })
{
  arr.push(function () { return i });
}

assert(arr[0]() === "aa");
assert(arr[1]() === "bb");
assert(arr[2]() === "cc");
