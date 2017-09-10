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

var str = "universe";
var res;

res = str.slice();
assert (res === "universe");

res = str.slice(1, 6);
assert (res === "niver");

res = str.slice("a", "-4");
assert (res === "univ");

res = str.slice(-5);
assert (res === "verse");

res = str.slice(-12, undefined);
assert (res === "universe");

res = str.slice(undefined, -20);
assert (res === "");

res = str.slice(undefined, undefined);
assert (res === "universe");

res = str.slice(Infinity, NaN);
assert (res === "");

res = str.slice(-Infinity, Infinity);
assert (res === "universe");

res = str.slice(NaN, -Infinity);
assert (res === "");

res = str.slice(false, true);
assert (res === "u");

var x;
res = str.slice(x, x);
assert (res === "universe");

var obj = {y: "foo"};
var arr = [x, x];
res = str.slice(obj, arr);
assert (res === "");
