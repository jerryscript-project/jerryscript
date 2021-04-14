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

var typedarrays = [
  new Uint8ClampedArray([1]),
  new Uint8Array([1]),
  new Uint16Array([1]),
  new Uint32Array([1]),
  new Float32Array([1]),
  new Float64Array([1]),
  new Int8Array([1]),
  new Int16Array([1]),
  new Int32Array([1]),
  new BigInt64Array([1n]),
  new BigUint64Array([1n]),
];

for (let ta of typedarrays) {
  for (let prop_name of [2, 5.1]) {
    var set_value = 4.2;

    if (ta.constructor === BigInt64Array || ta.constructor === BigUint64Array)
    {
      set_value = 4n;
    }

    (function () {
      "use strict";
      let set_result = ta[prop_name] = set_value;
      assert(set_result === set_value);
      assert(!ta.hasOwnProperty(prop_name));
      assert(ta.length === 1);
    })();

    (function () {
      let set_result = ta[prop_name] = set_value;
      assert(set_result === set_value);
      assert(!ta.hasOwnProperty(prop_name));
      assert(ta.length === 1);
    })();
  }
}
