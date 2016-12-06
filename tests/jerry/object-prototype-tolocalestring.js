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

var obj1 = {};
obj1.toString = function () { return "mystring"; }

assert (obj1.toLocaleString() === "mystring");

var obj2 = {a: 3};
assert (obj2.toLocaleString() === "[object Object]");


var obj3 = {toLocaleString: function() { throw ReferenceError ("foo"); }};
try {
  obj3.toLocaleString();

  assert (false);
} catch (e) {
  assert (e.message === "foo");
  assert (e instanceof ReferenceError);
}

// Test invalid toString call
var obj4 = {toString: 2};
try {
  obj4.toLocaleString();

  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

// Test undefined toString call
var obj5 = {};
var obj6;
obj5.toString = obj6
try {
  obj5.toLocaleString();

  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}
