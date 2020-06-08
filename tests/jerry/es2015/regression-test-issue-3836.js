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

function validate_typedarray (typedarray, result) {
  assert(typedarray.length === result.length);
  for (var i = 0; i < typedarray.length; i++) {
    assert(typedarray[i] === result[i]);
  }
}

var v1 = new Float64Array(6);
v1.buffer.constructor = Uint8Array;
var v2 = new Float64Array(v1);

assert(v2.buffer.constructor === Uint8Array);
validate_typedarray(v2, [0, 0, 0, 0, 0, 0]);

var v3 = new Uint32Array(6);
v3.buffer.constructor = Float64Array;
var v4 = new Uint8Array(v3);

assert(v4.buffer.constructor === Float64Array);
validate_typedarray(v4, [0, 0, 0, 0, 0, 0]);

var v5 = new Uint32Array(6);
v5.buffer.constructor = Set;
var v6 = new Uint8Array(v5);

assert(v6.buffer.constructor === Set);
validate_typedarray(v6, [0, 0, 0, 0, 0, 0]);

var species_called = false;

var v7 = new Float64Array(6);
var v8 = v7.buffer;
v8.constructor = {
  get [Symbol.species] (){
    species_called = true;
    return Uint8Array;
  }
}
var v9 = new Float64Array(v7);

assert(species_called);
assert(Reflect.getPrototypeOf(v9.buffer) === Uint8Array.prototype);
