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

var bigint64_array = new BigInt64Array([1n, 2n, 3n, -4n, 5n]);

function sum(prev, current) {
  return prev + current;
}

var bigint64_reduce_result = bigint64_array.reduce(sum, 7n);
assert(bigint64_reduce_result === 14n);

var biguint64_array = new BigUint64Array([1n, 2n, 3n, -4n, 5n]);
var biguint64_reduce_result = biguint64_array.reduce(sum, 7n);
assert(biguint64_reduce_result === 18446744073709551630n);
