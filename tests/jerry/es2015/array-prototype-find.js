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

var arrow_is_available = false;

try {
  assert (eval ("(f => 5) ()") === 5);
  arrow_is_available = true;
} catch (e) {
  assert (e instanceof SyntaxError);
}

var array1 = [5, 12, 0, 8, 130, 44];

function bigger_than_10 (element) {
  return element > 10;
}

assert (array1.find (bigger_than_10) === 12);

function less_than_0 (element) {
  if (element == 0) {
    throw new Error ("zero");
  }
  return element < 0;
}

try {
  array1.find (less_than_0);
  assert (false);
} catch (e) {
  assert (e instanceof Error);
  assert (e.message === "zero");
}

var inventory = [
    {name: 'apples', quantity: 2},
    {name: 'bananas', quantity: 0},
    {name: 'cherries', quantity: 5}
];

function isCherries (fruit) {
    return fruit.name === 'cherries';
}

assert (JSON.stringify (inventory.find (isCherries)) === '{"name":"cherries","quantity":5}');

if (arrow_is_available) {
  assert (eval ("inventory.find (fruit => fruit.name === 'bananas')") === inventory[1]);
}

/* Test the callback function arguments */
var src_array = [4, 6, 8, 12];
var array_index = 0;

function isPrime (element, index, array) {
  assert (array_index++ === index);
  assert (array === src_array)
  assert (element === array[index]);

  var start = 2;
  while (start <= Math.sqrt (element)) {
    if (element % start++ < 1) {
      return false;
    }
  }
  return element > 1;
}

assert (src_array.find (isPrime) === undefined);

src_array = [4, 5, 8, 12];
array_index = 0;
assert (src_array.find (isPrime) === 5);


// Checking behavior when unable to get length
var obj = {};
Object.defineProperty (obj, 'length', { 'get' : function () { throw new ReferenceError ("foo"); } });
obj.find = Array.prototype.find;

try {
  obj.find ();
  assert (false);
} catch (e) {
  assert (e.message === "foo");
  assert (e instanceof ReferenceError);
}

var data = { 0: 1, 2: -3, 3: "string" }
assert (Array.prototype.find.call (data, function (e) { return e < 5; }) === undefined);

// Checking behavior when unable to get element
var obj = {}
obj.length = 1;
Object.defineProperty (obj, '0', { 'get' : function () { throw new ReferenceError ("foo"); } });
obj.find = Array.prototype.find

try {
  obj.find (function () { return undefined; });
  assert (false);
} catch (e) {
  assert (e.message === "foo");
  assert (e instanceof ReferenceError);
}

// Checking behavior when the first argument is not a callback
var array = [1, 2, 3];

try {
  array.find (5);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

// Checking behavior when the first argument does not exist
try {
  array.find ();
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

// Checking behavior when the there are more than 2 arguments
assert (array.find (function (e) { return e < 2 }, {}, 8, 4, 5, 6, 6) === 1);

function func (element) {
  return element > 8;
}

/* ES v6.0 22.1.3.8.8.c
   Checking behavior when the first element deletes the second */
function f() { delete arr[1]; };
var arr = [0, 1, 2, 3];
Object.defineProperty(arr, '0', { 'get' : f });
Array.prototype.find.call(arr, func);
