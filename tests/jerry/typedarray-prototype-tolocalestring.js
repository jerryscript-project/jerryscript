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

var int_typedarrays = [
    new Uint8Array([0, 1, 2, 3, 4, 5]),
    new Uint16Array([0, 1, 2, 3, 4, 5]),
    new Uint32Array([0, 1, 2, 3, 4, 5]),
    new Int8Array([0, 1, 2, 3, 4, 5]),
    new Int16Array([0, 1, 2, 3, 4, 5]),
    new Int32Array([0, 1, 2, 3, 4, 5])
];

var float_typedarrays = [
    new Float32Array([0, 1, 2, 3, 4, 5]),
    new Float64Array([0, 1, 2, 3, 4, 5]),
];

int_typedarrays.forEach(function(e){
    // Test with normal inputs
    assert(e.toLocaleString() === "0,1,2,3,4,5");
    e.set([3, 3, 6, 1, 2, 9]);
    assert(e.toLocaleString() === "3,3,6,1,2,9");
    e.set([32, 12, 2, 8, 32, 1]);
    assert(e.toLocaleString() === "32,12,2,8,32,1");
    // Test with undefined
    e.set([12, undefined, 0, undefined, 3, 5]);
    assert(e.toLocaleString() === "12,0,0,0,3,5");
    // Test with null
    e.set([null, 3, 3, 3, null, 10]);
    assert(e.toLocaleString() === "0,3,3,3,0,10");
    e.set([12, null, 21, null, null, null]);
    assert(e.toLocaleString() === "12,0,21,0,0,0");
});

float_typedarrays.forEach(function(e){
    // Test with normal inputs
    assert(e.toLocaleString() === "0,1,2,3,4,5");
    e.set([3, 3, 6, 1, 2, 9]);
    assert(e.toLocaleString() === "3,3,6,1,2,9");
    e.set([32, 12, 2, 8, 32, 1]);
    assert(e.toLocaleString() === "32,12,2,8,32,1");
    // Test with undefined
    e.set([12, undefined, 0, undefined, 3, 5]);
    assert(e.toLocaleString() === "12,NaN,0,NaN,3,5");
    // Test with null
    e.set([null, 3, 3, 3, null, 10]);
    assert(e.toLocaleString() === "0,3,3,3,0,10");
    e.set([12, null, 21, null, null, null]);
    assert(e.toLocaleString() === "12,0,21,0,0,0");
});

// Test empty TypedArrays
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

empty_typedarrays.forEach(function(e){
    assert(e.toLocaleString() === "");
});

Number.prototype.toLocaleString = function() { return "5" }

var typedarrays = [
    new Uint8Array([10]),
    new Uint16Array([10]),
    new Uint32Array([10]),
    new Float32Array([10]),
    new Float64Array([10]),
    new Int8Array([10]),
    new Int16Array([10]),
    new Int32Array([10])
];

typedarrays.forEach(function(e){
    assert(e.toLocaleString() === "5");
});

Number.prototype.toLocaleString = "foo";

typedarrays.forEach(function(e){
    try {
        assert(e.toLocaleString() === "foo");
        assert(false);
    } catch (err) {
        assert(err instanceof TypeError);
    }
});
