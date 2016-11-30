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
obj1.prop = "hi";

assert (obj1.hasOwnProperty('prop') === true);
assert (obj1.hasOwnProperty('NO_PROP') === false);


// Test if the toString fails.
try {
  obj1.hasOwnProperty({toString: function() { throw new ReferenceError ("foo"); }});

  assert (false);
} catch (e) {
  assert (e.message === "foo");
  assert (e instanceof ReferenceError);
}

// Test if the toObject fails.

var obj2;
try {
  obj2.hasOwnProperty("fail");

  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

var obj_undef;
var obj3 = {};
Object.defineProperty(obj3, 'Test', { 'get' : function () {throw new ReferenceError ("foo"); } });
assert (obj3.hasOwnProperty("Test") === true);

Object.defineProperty(obj3, 'Test2', { 'get' : function () { return 0/0; } });
assert (obj3.hasOwnProperty("Test2") === true);

Object.defineProperty(obj3, 'Test4', { 'get' : function () { return obj_undef; } });
assert (obj3.hasOwnProperty("Test4") === true);
