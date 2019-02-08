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
  Array.prototype.entries.call (undefined);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError)
}

var array = ['a', "foo", 1, 1.5, true, {} ,[], function f () { }];

var iterator = array.entries ();

var current_item = iterator.next ();

for (var i = 0; i < array.length; i++) {
  assert (current_item.value[0] === i);
  assert (current_item.value[1] === array[i]);
  assert (current_item.done === false);

  current_item = iterator.next ();
}

assert (current_item.value === undefined);
assert (current_item.done === true);

function foo_error () {
  throw new ReferenceError ("foo");
}

array = [1, 2, 3, 4, 5, 6, 7];

['0', '3', '5'].forEach (function (e) {
  Object.defineProperty (array, e, { 'get' : foo_error });
})

iterator = array.entries ();

var expected_values = [2, 3, 5, 7];
var expected_indices = [1, 2, 4, 6];
var expected_values_idx = 0;

for (var i = 0; i < array.length; i++) {
  try {
    current_item = iterator.next ();
    assert (current_item.value[0] === expected_indices[expected_values_idx]);
    assert (current_item.value[1] === expected_values[expected_values_idx]);
    assert (current_item.done === false);
    expected_values_idx++;
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

iterator = array.entries ();
current_item = iterator.next ();

assert (current_item.value === undefined);
assert (current_item.done === true);

/* Test delete elements after the iterator has been constructed */

array = [0, 1, 2, 3, 4, 5];
iterator = array.entries ();

for (var i = 0; i < array.length; i++) {
  current_item = iterator.next ();
  assert (current_item.value[0] === i);
  assert (current_item.value[1] === array[i]);
  assert (current_item.done === false);
  array.pop();
}

assert ([].entries ().toString () === "[object Array Iterator]");
