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

assert (Math.cos.toString() === "function () { [native code] }");

var has_toString = none.toString() != "function () { /* ecmascript */ }"

function check(f, expected)
{
  assert (f.toString() === (has_toString ? expected : "function () { /* ecmascript */ }"))
}

function none() { return 1; }
check (none, "function none() { return 1; }")
assert (none.bind({}).toString() === "function () { [native code] }")

function single(b) { return 1; }
check (single, "function single(b) { return 1; }")
assert (single.bind({}).toString() === "function () { [native code] }")

function multiple(a,b) { return 1; }
check (multiple, "function multiple(a,b) { return 1; }")
assert (multiple.bind({}).toString() === "function () { [native code] }")

function lots(a,b,c,d,e,f,g,h,i,j,k) { return 1; }
check (lots, "function lots(a,b,c,d,e,f,g,h,i,j,k) { return 1; }")
assert (lots.bind({}).toString() === "function () { [native code] }")
