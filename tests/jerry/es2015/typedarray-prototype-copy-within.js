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
    new Uint8ClampedArray ([0, 1, 2, 3, 4, 5]),
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
        e.prototype.copyWithin.call (undefined);
        assert (false);
    } catch (err) {
        assert (err instanceof TypeError);
    }

    // Test with normal inputs
    assert(e.copyWithin(2, 1 ,4).toString() === '0,1,1,2,3,5');
    assert(e.copyWithin(3, 4, 6).toString() === '0,1,1,3,5,5');
    assert(e.copyWithin(4, 1, 3).toString() === '0,1,1,3,1,1');

    e.set([5, 2, 1, 3, 4, 4]);

    // Test with negative inputs
    assert(e.copyWithin(2, -10, 3).toString() === '5,2,5,2,1,4');
    assert(e.copyWithin(-3, 1, 6).toString() === '5,2,5,2,5,2');
    assert(e.copyWithin(2, 0, -3).toString() === '5,2,5,2,5,2');

    e.set([9, 3, 4, 5, 1, 7]);

    // Test with default inputs
    assert(e.copyWithin().toString() === '9,3,4,5,1,7');
    assert(e.copyWithin(3).toString() === '9,3,4,9,3,4');
    assert(e.copyWithin(1, 5).toString() === '9,4,4,9,3,4');

    e.set([12, 3, 1, 43, 2, 9]);

    // Test with too big inputs
    assert(e.copyWithin(2, 12, 21).toString() === '12,3,1,43,2,9');
    assert(e.copyWithin(4, 5, 13).toString() === '12,3,1,43,9,9');

    e.set([1, 2, 3, 1, 2, 1]);

    // Test with undefined inputs
    assert(e.copyWithin(undefined, 2, 4).toString() === '3,1,3,1,2,1');
    assert(e.copyWithin(undefined, undefined, 2).toString() === '3,1,3,1,2,1');
    assert(e.copyWithin(3, undefined, 5).toString() === '3,1,3,3,1,3');

    e.set([0, 2, 4, 2, 1, 5]);

    // Test with NaN/+-Infinity
    assert(e.copyWithin(NaN, 3, 5).toString() === '2,1,4,2,1,5');
    assert(e.copyWithin(3, Infinity, 5).toString() === '2,1,4,2,1,5');
    assert(e.copyWithin(1, 3, -Infinity).toString() === '2,1,4,2,1,5');
});
