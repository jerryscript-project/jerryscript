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

try {
  Array.prototype.values.call (undefined);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError)
}

var array = ['a', "foo", 1, 1.5, true, {} ,[], function f () { }];

var iterator = array.values ();

/* The initial value of the @@iterator property is the same function object
   as the initial value of the Array.prototype.values property. */
var symbol_iterator = array[Symbol.iterator] ();

var current_item = iterator.next ();
var symbol_current_item = symbol_iterator.next ();

for (var i = 0; i < array.length; i++) {
  assert (current_item.value === array[i]);
  assert (current_item.done === false);

  assert (current_item.value === symbol_current_item.value);
  assert (current_item.done === symbol_current_item.done);

  current_item = iterator.next ();
  symbol_current_item = symbol_iterator.next ();
}

assert (current_item.value === undefined);
assert (current_item.done === true);
assert (current_item.value === symbol_current_item.value);
assert (current_item.done === symbol_current_item.done);

function foo_error () {
  throw new ReferenceError ("foo");
}

array = [1, 2, 3, 4, 5, 6, 7];

['0', '3', '5'].forEach (function (e) {
  Object.defineProperty (array, e, { 'get' : foo_error });
})

iterator = array.values ();

var expected_values = [2, 3, 5, 7];
var expected_values_idx = 0;

for (var i = 0; i < array.length; i++) {
  try {
    current_item = iterator.next ();
    assert (current_item.value === expected_values[expected_values_idx++]);
    assert (current_item.done === false);
  } catch (e) {
    assert (e instanceof ReferenceError);
    assert (e.message === "foo");
  }
}

current_item = iterator.next ();
assert (current_item.value === undefined);
assert (current_item.done === true);

/* Test empty array */
array = [];

iterator = array.values ();
current_item = iterator.next ();

assert (current_item.value === undefined);
assert (current_item.done === true);

/* Test delete elements after the iterator has been constructed */

array = [0, 1, 2, 3, 4, 5];
iterator = array.values ();

for (var i = 0; i < array.length; i++) {
  current_item = iterator.next ();
  assert (current_item.value === array[i]);
  assert (current_item.done === false);
  array.pop();
}

assert ([].values ().toString () === "[object Array Iterator]");
