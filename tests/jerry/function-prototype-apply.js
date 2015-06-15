// Copyright 2015 Samsung Electronics Co., Ltd.
// Copyright 2015 University of Szeged.
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

// See a general usage: number addition.
function addNum ()
{
  var sum = 0;
  for(var i = 0; i < arguments.length; i++)
  {
    sum += arguments[i];
  }
  return sum;
}

var array = [6720, 4, 42];
var obj;

obj = addNum.apply(obj, array);
assert (obj === 6766);

// If the arguments are missing.
obj = addNum.apply();
assert (obj === 0);

obj = addNum.apply(obj);
assert (obj === 0);

// If the function is a builtin.
assert (Math.min.apply(null, array) === 4);
assert (Math.max.apply(null, array) === 6720);

// If the function can't be used as caller.
try {
  obj = new Function.prototype.apply();
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

// If the called function throws an error.
function throwError ()
{
  throw new ReferenceError ("foo");
}

try {
  obj = throwError.apply(obj, array);
  assert (false);
} catch (e) {
  assert (e.message === "foo");
  assert (e instanceof ReferenceError);
}

// If the array access throws an error.
Object.defineProperty(array, '0', { 'get' : function () { throw new ReferenceError ("foo"); } });

try {
  obj = addNum.apply(obj, array);
  assert (false);
} catch (e) {
  assert (e.message === "foo");
  assert (e instanceof ReferenceError);
}
