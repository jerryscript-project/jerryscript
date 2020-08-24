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

var calls = 0;
Number.prototype.toLocaleString = function() {
  return {
    toString: function() {
      calls++;
      if (calls > 1) {
        throw "ERROR V";
      }
    }
  };
};

var array = [42.333333, 2.3];

var sampleA = new Float32Array(array);
try {
  sampleA.toLocaleString();
} catch(ex) {
  assert(ex === "ERROR V");
}
assert(calls === 2);

var sampleB = new Uint8Array(array);
try {
  sampleB.toLocaleString();
} catch(ex) {
  assert(ex === "ERROR V");
}
assert(calls === 3);
