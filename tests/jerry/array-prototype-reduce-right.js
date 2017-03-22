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

// This test will not pass on FLOAT32 due to precision issues

var func = function(a, b) {
  return a + b;
}

// check function type
try {
  [0].reduceRight(new Object());
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

// check for init value
try {
  [].reduceRight(func);
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

try {
  var arg2;
  [].reduceRight(func, arg2);
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

try {
  var a = new Array();
  a.length = 10;
  a.reduceRight(func);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError)
}

// various checks
assert([].reduceRight(func, 1) === 1);

assert([0].reduceRight(func) === 0);

assert([0, 1].reduceRight(func) === 1);

assert([0, 1].reduceRight(func, 1) === 2);

assert([0, 1, 2, 3].reduceRight(func, 1) === 7);

assert (["A","B"].reduceRight(func) === "BA");

assert (["A","B"].reduceRight(func, "Init:") === "Init:BA");

assert ([0, 1].reduceRight(func, 3.2) === 4.2);

assert ([0, "x", 1].reduceRight(func) === "1x0");

assert ([0, "x", 1].reduceRight(func, 3.2) === "4.2x0");

var long_array = [0, 1];
assert (long_array.reduceRight(func,10) === 11);

long_array[10000] = 1;
assert (long_array.reduceRight(func,10) === 12);

var accessed = false;
function callbackfn(prevVal, curVal, idx, obj) {
    accessed = true;
    return typeof prevVal === "undefined";
}

var obj = { 0: 11, length: 1 };

assert (Array.prototype.reduceRight.call(obj, callbackfn, undefined) === true && accessed);
