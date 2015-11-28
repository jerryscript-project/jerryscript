// Copyright 2015 Samsung Electronics Co., Ltd.
// Copyright 2015 University of Szeged.
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

assert(parseFloat("1") === 1);
assert(parseFloat("+1") === 1);
assert(parseFloat("-1") === -1);
assert(parseFloat("1.2") === 1.2);
assert(parseFloat("+1.2") === 1.2);
assert(parseFloat("-1.2") === -1.2);
assert(parseFloat("1.2e3") === 1200);
assert(parseFloat("+1.2e3") === 1200);
assert(parseFloat("-1.2e3") === -1200);
assert(parseFloat("   \n\t  1.2e3") === 1200);
assert(parseFloat("03.02e1") === 30.2);
assert(parseFloat("003.") === 3);
assert(parseFloat(".2e3") === 200);
assert(parseFloat("1.e3") === 1000);
assert(parseFloat("1.2e") === 1.2);
assert(parseFloat("1.e") === 1);
assert(parseFloat("1.e3") === 1000);
assert(parseFloat("1e3") === 1000);
assert(parseFloat("1e") === 1);
assert(parseFloat("1.2e3foo") === 1200);
assert(isNaN(parseFloat("foo1.2e3foo")));
assert(parseFloat("Infinity") === Infinity);
assert(parseFloat("-Infinity") === -Infinity);
assert(parseFloat("Infinityfoo") === Infinity);
assert(parseFloat("-Infinityfoo") === -Infinity);
assert(isNaN(parseFloat("")));
assert(isNaN(parseFloat(".")));
assert(isNaN(parseFloat("..")));
assert(isNaN(parseFloat("+")));
assert(isNaN(parseFloat("-")));
assert(isNaN(parseFloat("e")));
assert(isNaN(parseFloat("a")));
assert(isNaN(parseFloat("e+")));
assert(isNaN(parseFloat("+e-")));
assert(isNaN(parseFloat(".e")));
assert(isNaN(parseFloat(".a")));
assert(isNaN(parseFloat("e3")));
assert(isNaN(parseFloat(".e3")));
assert(parseFloat("1..2") === 1);
assert(parseFloat("1.2.3") === 1.2);
assert(parseFloat("1.2ee3") === 1.2);
assert(parseFloat("0") === 0);
assert(parseFloat(".0") === 0);
assert(parseFloat("0.e3") === 0);
assert(parseFloat("0.0e3") === 0);
assert(parseFloat("1.2eA") === 1.2);
assert(parseFloat("1.ae3") === 1);
assert(parseFloat("\u00a0\u00a01.2e3") === 1200);
assert(parseFloat("\u2029\u2029\u00a01.2e\u00D0") === 1.2);
assert(isNaN(parseFloat("\u2029\u2029\u00a0\u00D01.2e3")));
assert(parseFloat("\u2029\u2029\u00a01.\u20292e\u00D0") === 1);
assert(isNaN(parseFloat("\u2029\u2029")));

var obj = new Object();
var arr = [3,4,5];
var num = 7;
var bool = true;
var undef;

assert(isNaN(parseFloat(obj)));
assert(parseFloat(arr) === 3);
assert(parseFloat(num) === 7);
assert(isNaN(parseFloat(bool)));
assert(isNaN(parseFloat(undef)));

var obj = { toString : function () { throw new ReferenceError("foo") } };
try {
  parseFloat(obj);
  assert(false);
} catch (e) {
  assert(e instanceof ReferenceError);
  assert(e.message === "foo");
}
