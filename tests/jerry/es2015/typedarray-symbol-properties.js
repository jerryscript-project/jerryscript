/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

var typedArrayTypes = [Int8Array,
                       Uint8Array,
                       Uint8ClampedArray,
                       Int16Array,
                       Uint16Array,
                       Int32Array,
                       Uint32Array,
                       Float32Array,
                       Float64Array];

typedArrayTypes.forEach (function (typedArrayType) {
  var typedArray = new typedArrayType ();
  var fooSymbol = Symbol ("foo");
  var barSymbol = Symbol ("bar");
  typedArray[fooSymbol] = 5;
  assert (typedArray[fooSymbol] === 5);

  Object.defineProperty (typedArray, barSymbol, {value: 10});
  assert (typedArray[barSymbol] === 10);
});
