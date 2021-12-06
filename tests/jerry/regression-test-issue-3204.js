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

(function tc3204_1 () {
  (new Int16Array(7029)).subarray(5812).reduce(function() { })
})();

(function tc3204_2 () {
  (new (Uint8Array)((new (ArrayBuffer)("5")), EvalError.length)).reduceRight(function() { })
})();

(function tc3204_3 () {
  var v0 = (((new ((new ((new Object).constructor)).constructor)).constructor)("")).split( )
  var $ = (new Float64Array(7652)).subarray(1872).reduce((new (v0.filter.constructor)))
})();
