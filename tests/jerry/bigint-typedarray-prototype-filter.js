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

function arrayEquals(result, expected) {
  assert(result.length === expected.length);

  for (var idx = 0; idx < result.length; idx++) {
    assert(result[idx] === expected[idx]);
  }
}

var bigint64_array = new BigInt64Array([1n, 2n, 3n, -4n, 5n]);

function positive(element, index, array) {
  return element > 0n;
}

var bigint64_filter = bigint64_array.filter(positive);
arrayEquals(bigint64_filter, [1n, 2n, 3n, 5n]);

var biguint64_array = new BigUint64Array([1n, 2n, 3n, -4n, 5n]);
var biguint64_filter = biguint64_array.filter(positive);
assert(biguint64_filter.length === 5);
