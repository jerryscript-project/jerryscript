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

// check properties
assert(Object.getOwnPropertyDescriptor(String.prototype.trim, 'length').configurable === false);

assert(Object.getOwnPropertyDescriptor(String.prototype.trim, 'length').enumerable === false);

assert(Object.getOwnPropertyDescriptor(String.prototype.trim, 'length').writable === false);

assert(String.prototype.trim.length === 0);

// check this value
assert(String.prototype.trim.call(new String()) === "");

assert(String.prototype.trim.call({}) === "[object Object]");

// check undefined
try {
  String.prototype.trim.call(undefined);
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

// check null
try {
  String.prototype.trim.call(null);
  assert(false);
} catch(e) {
  assert(e instanceof TypeError);
}

// simple checks
assert(" hello world".trim() === "hello world");

assert("hello world ".trim() === "hello world");

assert("    hello world   ".trim() === "hello world");

assert("\t  hello world\n".trim() === "hello world");

assert("\t\n  hello world\t \n ".trim() === "hello world");

assert("hello world\n   \t\t".trim() === "hello world");

assert(" hello world \\ ".trim() === "hello world \\");

assert("**hello world**".trim() === "**hello world**");

assert(" \t \n".trim() === "");

assert("          ".trim() === "");

assert("".trim() === "");

assert("\uf389".trim() === "\uf389");
assert(String.prototype.trim.call('\uf389') === "\uf389");
assert("\u20291\u00D0".trim() === "1\u00D0");
assert("\u20291\u00A0".trim() === "1");

assert("\u0009\u000B\u000C\u0020\u00A01".trim() === "1");
assert("\u000A\u000D\u2028\u202911".trim() === "11");

assert("\u0009\u000B\u000C\u0020\u00A01\u0009\u000B\u000C\u0020\u00A0".trim() === "1");
assert("\u000A\u000D\u2028\u202911\u000A\u000D\u2028\u2029".trim() === "11");
