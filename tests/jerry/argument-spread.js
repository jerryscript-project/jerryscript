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

// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Spread_syntax

function mustThrow(str) {
  try {
    eval(str);
    assert(false);
  } catch (e) {
    assert(e instanceof TypeError);
  }
}

function assertArrayEqual(actual, expected) {
  assert(actual.length === expected.length);

  for (var i = 0; i < actual.length; i++) {
    assert(actual[i] === expected[i]);
  }
}

// Spread syntax
function sum(x, y, z) {
  return x + y + z;
}

const numbers = [1, 2, 3];

assert(sum(...numbers) === 6);

// Replace apply()
function myFunction (v, w, x, y, z) {
  return v + w + x + y + z;
}
var args = [0, 1];

assert(myFunction (-1, ...args, 2, ...[3]) == 5);

// Apply for new
var dateFields = [1970, 0, 1];
var d = new Date(...dateFields);

assert(d.toString().substring(0, 24) === "Thu Jan 01 1970 00:00:00");

function applyAndNew(constructor, args) {
   function partial () {
      return constructor.apply (this, args);
   };
   if (typeof constructor.prototype === "object") {
      partial.prototype = Object.create(constructor.prototype);
   }
   return partial;
}

function myConstructor () {
   assertArrayEqual(arguments, myArguments);
   this.prop1 = "val1";
   this.prop2 = "val2";
};

var myArguments = ["hi", "how", "are", "you", "mr", null];
var myConstructorWithArguments = applyAndNew(myConstructor, myArguments);

var obj = new myConstructorWithArguments;
assert(Object.keys(obj).length === 2);
assert(obj.prop1 === "val1");
assert(obj.prop2 === "val2");

// Test spread prop call
var o = { f(a,b,c) { return a + b + c },
          g(a,b) { throw new TypeError ("5") }
        };

assert (o.f(...["a", "b", "c"]) === "abc");

mustThrow ("o.g (...[1,2])")

// Test spread super call
class MyArray extends Array {
  constructor(...args) {
    super(...args);
  }
}

var array = new MyArray(1, 2, 3);
assertArrayEqual(array, [1,2,3]);
assert(array instanceof MyArray);
assert(array instanceof Array);

function argumentOrderTest() {
  var result = []
  for (i = 0; i < arguments.length; i++) {
      result.push(arguments[i]);
  }

  return result;
}

assertArrayEqual(argumentOrderTest(1, 2, ...[3, 4]), [1, 2, 3, 4]);
