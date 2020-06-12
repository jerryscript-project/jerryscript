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

var int_typedarrays = [
    Uint8ClampedArray,
    Uint8Array,
    Uint16Array,
    Uint32Array,
    Int8Array,
    Int16Array,
    Int32Array
];

var float_typedarrays = [
    Float32Array,
    Float64Array,
];

var obj = {
    0: 0,
    1: 1
};

int_typedarrays.forEach(function(e){
    try {
        e.of.call(undefined);
        assert (false);
    } catch (err) {
        assert (err instanceof TypeError);
    }

    try {
        e.of.call(obj);
        assert (false);
    } catch (err) {
        assert (err instanceof TypeError);
    }

    // Test with regulat inputs
    assert(e.of(1,2,3).toString() === "1,2,3");
    assert(e.of(12,4,5,0).toString() === "12,4,5,0");
    assert(e.of(23).toString() === "23");

    // Test with string inputs
    assert(e.of("12",4).toString() === "12,4");
    assert(e.of(1,2,"foo").toString() === "1,2,0");

    // Test with undefined
    assert(e.of(undefined).toString() === "0");

    // Test with object
    assert(e.of(obj).toString() === "0");

    // Test with NaN
    assert(e.of(NaN).toString() === "0");
});

float_typedarrays.forEach(function(e){
    try {
        e.of.call(undefined);
        assert (false);
    } catch (err) {
        assert (err instanceof TypeError);
    }

    try {
        e.of.call(obj);
        assert (false);
    } catch (err) {
        assert (err instanceof TypeError);
    }

    // Test with regulat inputs
    assert(e.of(1,2,3).toString() === "1,2,3");
    assert(e.of(12,4,5,0).toString() === "12,4,5,0");
    assert(e.of(23).toString() === "23");

    // Test with string inputs
    assert(e.of("12",4).toString() === "12,4");
    assert(e.of(1,2,"foo").toString() === "1,2,NaN");
    assert(e.of("-12").toString() === "-12");

    // Test with undefined
    assert(e.of(undefined).toString() === "NaN");

    // Test with object
    assert(e.of(obj).toString() === "NaN");

    // Test with NaN
    assert(e.of(NaN).toString() === "NaN");
});

// Test with negative inputs
var a = Uint8ClampedArray.of(-8);
assert(a.toString() === "0");

a = Uint8Array.of(-8);
assert(a.toString() === "248");

a = Uint16Array.of(-8);
assert(a.toString() === "65528");

a = Uint32Array.of(-8);
assert(a.toString() === "4294967288");

a = Int8Array.of(-8);
assert(a.toString() === "-8");

a = Int16Array.of(-8);
assert(a.toString() === "-8");

a = Int32Array.of(-8);
assert(a.toString() === "-8");

a = Float32Array.of(-8);
assert(a.toString() === "-8");

a = Float64Array.of(-8);
assert(a.toString() === "-8");
