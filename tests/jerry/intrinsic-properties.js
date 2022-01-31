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

assert(Array.prototype.values === Array.prototype[Symbol.iterator]);
assert(Set.prototype.values === Set.prototype[Symbol.iterator]);
assert(Set.prototype.keys === Set.prototype[Symbol.iterator]);
assert(Map.prototype.entries === Map.prototype[Symbol.iterator]);
assert(Date.prototype.toGMTString === Date.prototype.toUTCString);
assert(Number.parseInt === parseInt);
assert(Number.parseFloat === parseFloat);

var typedarrays = [Uint8Array, Uint16Array, Uint32Array, Int8Array, Int16Array, Int32Array, Float32Array, Float64Array];
for (ta1 of typedarrays) {
  for (ta2 of typedarrays) {
    assert(ta1.prototype.values === ta2.prototype[Symbol.iterator]);
  }

  assert(ta1.prototype.toString === Array.prototype.toString);
}
