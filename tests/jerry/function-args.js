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

function f1(x)
{
  var c1 = (x >= 1);
  var c2 = (x <= 10);

  if (c1 === true)
  {
    if (c2 === true)
    {
      assert(t);

      return;
    }
  }

  assert(t === false);
}

var i;
var t = true;

for(i = 1; i <= 10; i++)
{
  f1(i);
}

t = false;

for(i = 11; i <= 20; i++)
{
  f1(i);
}

function g (p, p) {
  assert (p === arguments[1]);
  assert (p === 'second');
}

g ('first', 'second');

try {
  f1 ({});
  f1 (undefined_variable);
  assert (false);
}
catch (e) {
  assert (e instanceof ReferenceError);
}

function f2 ()
{
  return this;
}

with ({})
{
  assert (f2 () === this);
}
