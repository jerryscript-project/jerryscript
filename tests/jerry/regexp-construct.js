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

var r;

r = new RegExp ();
assert (r.source == "(?:)");
assert (r.global == false);
assert (r.ignoreCase == false);
assert (r.multiline == false);

r = new RegExp ("a");
assert (r.source == "a");
assert (r.global == false);
assert (r.ignoreCase == false);
assert (r.multiline == false);

r = new RegExp ("a","gim");
assert (r.source == "a");
assert (r.global == true);
assert (r.ignoreCase == true);
assert (r.multiline == true);

r = RegExp ("a");
assert (r.source == "a");
assert (r.global == false);
assert (r.ignoreCase == false);
assert (r.multiline == false);

r = RegExp ("a","gim");
assert (r.source == "a");
assert (r.global == true);
assert (r.ignoreCase == true);
assert (r.multiline == true);

var r2;
try {
  r2 = RegExp (r,"gim");
  assert(false);
}
catch ( e )
{
  assert (e instanceof TypeError);
}

r2 = RegExp (r);
assert (r2.source == "a");
assert (r2.global == true);
assert (r2.ignoreCase == true);
assert (r2.multiline == true);

r2 = RegExp (r, undefined);
assert (r2.source == "a");
assert (r2.global == true);
assert (r2.ignoreCase == true);
assert (r2.multiline == true);

r = /(?:)/;
assert (r.source == "(?:)");
assert (r.global == false);
assert (r.ignoreCase == false);
assert (r.multiline == false);

r = /a/;
assert (r.source == "a");
assert (r.global == false);
assert (r.ignoreCase == false);
assert (r.multiline == false);

r = /a/gim;
assert (r.source == "a");
assert (r.global == true);
assert (r.ignoreCase == true);
assert (r.multiline == true);

assert(Object.prototype.toString.call(RegExp.prototype) === '[object RegExp]');

/* The 'undefined' argument for the RegExp constructor should not be converted to string,
 * and it should behave just like when there is no argument.
 */
r1 = new RegExp();
r2 = new RegExp(undefined);
var foo;
r3 = new RegExp(foo)

assert (r1.source === r2.source);
assert (r2.source === r3.source);

r = new RegExp ("foo", undefined);
assert (r.source === "foo");
assert (r.global === false);
assert (r.ignoreCase === false);
assert (r.multiline === false);

r = new RegExp ("foo", void 0);
assert (r.source === "foo");
assert (r.global === false);
assert (r.ignoreCase === false);
assert (r.multiline === false);

try {
  new RegExp (undefined, "ii");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

try {
  new RegExp ("", "gg");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

try {
  new RegExp (void 0, "mm");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

r = new RegExp (undefined, undefined);
assert (r.source == "(?:)");
assert (r.global == false);
assert (r.ignoreCase == false);
assert (r.multiline == false);

/* RegExp properties */
assert (RegExp.length === 2);
assert (RegExp.prototype.source === "(?:)");
assert (RegExp.prototype.global === false);
assert (RegExp.prototype.ignoreCase === false);
assert (RegExp.prototype.multiline === false);

RegExp.prototype.source = "a";
RegExp.prototype.global = true;
RegExp.prototype.ignoreCase = true;
RegExp.prototype.multiline = true;
assert (RegExp.prototype.source === "(?:)");
assert (RegExp.prototype.global === false);
assert (RegExp.prototype.ignoreCase === false);
assert (RegExp.prototype.multiline === false);

delete RegExp.prototype.source;
delete RegExp.prototype.global;
delete RegExp.prototype.ignoreCase;
delete RegExp.prototype.multiline;
assert (RegExp.prototype.source === "(?:)");
assert (RegExp.prototype.global === false);
assert (RegExp.prototype.ignoreCase === false);
assert (RegExp.prototype.multiline === false);
