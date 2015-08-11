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

assert (x === undefined);
assert (y === undefined);
assert (z === undefined);
assert (i === undefined);
assert (j === undefined);
assert (k === undefined);
assert (q === undefined);
assert (v === undefined);

eval ('var n');
eval ('var m = 1');

try
{
  x = p;
  assert (false);
}
catch (e)
{
  assert (e instanceof ReferenceError);
}

{
  var y;
}
var x = y;

do var z; while (0);

for (var i, j = function () {var p;}; i === undefined; i = null)
{
}

for (var q in {})
{
}

{ var v = 1 }

try
{
  var k
  l
  assert (false)
}
catch (e)
{
  assert (e instanceof ReferenceError);
}
