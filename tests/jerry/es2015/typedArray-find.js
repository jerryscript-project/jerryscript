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

var typedArrayTypes = [Int8Array,
                       Uint8Array,
                       Uint8ClampedArray,
                       Int16Array,
                       Uint16Array,
                       Int32Array,
                       Uint32Array,
                       Float32Array,
                       Float64Array];


typedArrayTypes.forEach (function (TypedArray) {

  var array1 = new TypedArray ([5, 12, 0, 8, 120, 44]);

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

  if (arrow_is_available) {
      assert (eval ("array1.find (e => e > 100)") === 120);
  }

  /* Test the callback function arguments */
  var src_array = new TypedArray ([4, 6, 8, 12]);
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

  src_array = new TypedArray ([4, 5, 8, 12]);
  array_index = 0;
  assert (src_array.find (isPrime) === 5);

  // Checking behavior when the given object is not %TypedArray%
  try {
    TypedArray.prototype.find.call (5);
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  // Checking behavior when the first argument is not a callback
  var array = new TypedArray ([1, 2, 3]);

  try {
    array.find (5);
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  // Checking behavior when the first argument is not exists
  try {
    array.find ();
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  // Checking behavior when the there are more than 2 arguments
  assert (array.find (function (e) { return e < 2 }, {}, 8, 4, 5, 6, 6) === 1);
})
