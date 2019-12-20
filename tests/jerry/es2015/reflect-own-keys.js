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

// Copyright 2015 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

var object1 = {
  property1: 42,
  property2: 13
};

var array1 = [];

assert (Reflect.ownKeys (object1)[0] === "property1");
assert (Reflect.ownKeys (object1)[1] === "property2");
assert (Reflect.ownKeys (object1).length === 2);

var obj = { a: 1, b: 2};
var keys = Reflect.ownKeys (obj);
assert (2 === keys.length);
assert ("a" === keys[0]);
assert ("b" === keys[1]);

var obj = { a: function(){}, b: function(){} };
var keys = Reflect.ownKeys (obj);
assert (2 === keys.length);
assert ("a" === keys[0]);
assert ("b" === keys[1]);

// Check slow case
var obj = { a: 1, b: 2, c: 3 };
delete obj.b;
var keys = Reflect.ownKeys (obj)
assert (2 === keys.length);
assert ("a" === keys[0]);
assert ("c" === keys[1]);

// Check that non-enumerable properties are being returned.
var keys = Reflect.ownKeys ([1, 2]);
assert (3 === keys.length);
assert ("0" === keys[0]);
assert ("1" === keys[1]);
assert ("string" === typeof keys[0]);
assert ("string" === typeof keys[1]);
assert ("length" === keys[2]);

// Check that no proto properties are returned.
var obj = { foo: "foo" };
var obj2 = { bar: "bar" };
Object.setPrototypeOf (obj, obj2)
keys = Reflect.ownKeys (obj);
assert (1 === keys.length);
assert ("foo" === keys[0]);

// Check that getter properties are returned.
var obj = {};
Object.defineProperty (obj, "getter", function() {})
//obj.__defineGetter__("getter", function() {});
keys = Reflect.ownKeys (obj);
assert (1 === keys.length);
assert ("getter" === keys[0]);

// Check that implementation does not access Array.prototype.
var savedConcat = Array.prototype.concat;
Array.prototype.concat = function() { return []; }
keys = Reflect.ownKeys ({0: 'foo', bar: 'baz'});
assert (2 === keys.length);
assert ('0' === keys[0]);
assert ('bar'=== keys[1]);
assert (Array.prototype === Object.getPrototypeOf (keys))
Array.prototype.concat = savedConcat;

try {
  Reflect.ownKeys (4);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  Reflect.ownKeys("cica");
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  Reflect.ownKeys(true);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

assert (Reflect.ownKeys (Object (4)) !== [])
assert (Reflect.ownKeys (Object ("foo")) !== ["0", "1", "2", "length"])
assert (Reflect.ownKeys (Object (4)) !== []);
assert (Reflect.ownKeys (Object ("foo")) !== [0, 1, 2, "length"]);
assert (Reflect.ownKeys (Object (true)) !== []);

var id = Symbol("my kitten");
var user = {
  name: "Bob",
  age:  30,
  [id]: "is batcat"
}

assert (Reflect.ownKeys (user)[0] === "name");
assert (Reflect.ownKeys (user)[1] === "age");
assert (Reflect.ownKeys (user)[2] === id);
