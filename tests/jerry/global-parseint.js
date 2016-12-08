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

assert(parseInt("123") === 123);
assert(parseInt("+123") === 123);
assert(parseInt("-123") === -123);
assert(parseInt("0123") === 123);
assert(parseInt("  123") === 123);
assert(parseInt(" \n  123") === 123);
assert(parseInt(" \n  123  \t") === 123);
assert(parseInt("0x123") === 291);
assert(parseInt("0X123") === 291);
assert(parseInt("123", 4) === 27);
assert(parseInt("ABC", 16) === 2748);
assert(parseInt("12A3") === 12);
assert(parseInt("12.34") === 12);
assert(isNaN(parseInt("AB")));
assert(isNaN(parseInt("")));
assert(isNaN(parseInt("-")));
assert(isNaN(parseInt("-", 11)));
assert(parseInt("\u00a0123") === 123);
assert(parseInt("\u20291  123\u00D0") === 1);
assert(parseInt("\u00a0123", 13) === 198);
assert(parseInt("\u2029123  1\u00D0", 11) === 146);
assert(isNaN(parseInt("\u0009")));
assert(isNaN(parseInt("\u00A0")));
assert(parseInt("\u00A0\u00A0-1") === parseInt("-1"));
assert(parseInt("\u00A01") === parseInt("1"));

var bool = true;
var obj = new Object();
var num = 8;
var arr = [2,3,4];
var undef;

assert(isNaN(parseInt(bool, bool)));
assert(isNaN(parseInt(bool, obj)));
assert(isNaN(parseInt(bool, num)));
assert(isNaN(parseInt(bool, arr)));

assert(isNaN(parseInt(obj, bool)));
assert(isNaN(parseInt(obj, obj)));
assert(isNaN(parseInt(obj, num)));
assert(isNaN(parseInt(obj, arr)));

assert(isNaN(parseInt(num, bool)));
assert(parseInt(num, obj) === 8);
assert(isNaN(parseInt(num, num)));
assert(parseInt(num, arr) === 8);

assert(isNaN(parseInt(arr, bool)));
assert(parseInt(arr, obj) === 2);
assert(parseInt(arr, num) === 2);
assert(parseInt(arr, arr) === 2);

assert(isNaN(parseInt(undef, bool)));
assert(isNaN(parseInt(undef, obj)));
assert(isNaN(parseInt(undef, num)));
assert(isNaN(parseInt(undef, arr)));

var obj = { toString : function () { throw new ReferenceError("foo") } };
try {
  parseInt(obj);
  assert(false);
} catch (e) {
  assert(e instanceof ReferenceError);
  assert(e.message === "foo");
}
