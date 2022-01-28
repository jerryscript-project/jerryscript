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

var normal_typedarrays = [
  new Uint8Array([1, 2, 3, 4, 5, 6, 7, 8]),
  new Uint16Array([1, 2, 3, 4, 5, 6, 7, 8]),
  new Uint32Array([1, 2, 3, 4, 5, 6, 7, 8]),
  new Float32Array([1, 2, 3, 4, 5, 6, 7, 8]),
  new Float64Array([1, 2, 3, 4, 5, 6, 7, 8]),
  new Int8Array([1, 2, 3, 4, 5, 6, 7, 8]),
  new Int16Array([1, 2, 3, 4, 5, 6, 7, 8]),
  new Int32Array([1, 2, 3, 4, 5, 6, 7, 8])
];

normal_typedarrays.forEach (function(e){
  try {
    e.prototype.entries.call (undefined);
    assert (false);
  }
  catch (e) {
    assert (e instanceof TypeError)
  }
});

normal_typedarrays.forEach(function(e){
  var iterator = e.entries ();
  var current_item = iterator.next ();

  for (var i = 0; i < e.length; i++) {
    assert (current_item.value[0] === i);
    assert (current_item.value[1] === e[i]);
    assert (current_item.done === false);

    current_item = iterator.next ();
  }

  assert (current_item.value === undefined);
  assert (current_item.done === true);
});

var empty_typedarrays = [
  new Uint8Array([]),
  new Uint16Array([]),
  new Uint32Array([]),
  new Float32Array([]),
  new Float64Array([]),
  new Int8Array([]),
  new Int16Array([]),
  new Int32Array([])
];

empty_typedarrays.forEach(function (e){
  iterator = e.entries ();
  current_item = iterator.next ();

  assert (current_item.value === undefined);
  assert (current_item.done === true);
});

assert ([].entries ().toString () === "[object Array Iterator]");

var bigint_array = new BigInt64Array([1n, 2n, 3n, 4n]);
var entries = bigint_array.entries();
assert(entries.next().value[1] === 1n);
assert(entries.next().value[1] === 2n);

var biguint_array = new BigUint64Array([1n, 2n, 3n, 4n]);
var entries = biguint_array.entries();
assert(entries.next().value[1] === 1n);
assert(entries.next().value[1] === 2n);
