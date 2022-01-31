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

var typedarrays = [
    new Uint8ClampedArray([0, 1, 2, 3, 4, 5]),
    new Uint8Array([0, 1, 2, 3, 4, 5]),
    new Uint16Array([0, 1, 2, 3, 4, 5]),
    new Uint32Array([0, 1, 2, 3, 4, 5]),
    new Float32Array([0, 1, 2, 3, 4, 5]),
    new Float64Array([0, 1, 2, 3, 4, 5]),
    new Int8Array([0, 1, 2, 3, 4, 5]),
    new Int16Array([0, 1, 2, 3, 4, 5]),
    new Int32Array([0, 1, 2, 3, 4, 5])
  ];

typedarrays.forEach(function(e){
    try {
        e.prototype.slice.call (undefined);
        assert (false);
    } catch (err) {
        assert (err instanceof TypeError);
    }

    // Test with normal inputs
    assert(e.slice(1, 3).toString() === "1,2");
    assert(e.slice(2, 5).toString() === "2,3,4");
    assert(e.slice(0, 6).toString() === "0,1,2,3,4,5");

    // Test witn negative inputs
    assert(e.slice(-2, 5).toString() === "4");
    assert(e.slice(0, -3).toString() === "0,1,2");
    assert(e.slice(-1, -4).toString() === "");

    // Test with bigger inputs then length
    assert(e.slice(7, 1).toString() === "");
    assert(e.slice(2, 9).toString() === "2,3,4,5");

    // Test with undefined
    assert(e.slice(undefined, 4).toString() === "0,1,2,3");
    assert(e.slice(0, undefined).toString() === "0,1,2,3,4,5");
    assert(e.slice(undefined, undefined).toString() === "0,1,2,3,4,5");

    // Test with NaN and +/-Infinity
    assert(e.slice(NaN, 3).toString() === "0,1,2");
    assert(e.slice(2, Infinity).toString() === "2,3,4,5");
    assert(e.slice(-Infinity, Infinity).toString() === "0,1,2,3,4,5");

    // Test with default inputs
    assert(e.slice().toString() === "0,1,2,3,4,5");
    assert(e.slice(4).toString() === "4,5");
});
