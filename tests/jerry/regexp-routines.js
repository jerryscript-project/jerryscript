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

r = new RegExp ("a");
assert (r.exec ("a") == "a");
assert (r.exec ("b") == null);
try {
  r.exec.call({}, "a");
  assert (false)
}
catch (e)
{
  assert (e instanceof TypeError);
}

assert (r.test ("a") == true);
assert (r.test ("b") == false);
try {
  r.test.call({}, "a");
  assert (false)
}
catch (e)
{
  assert (e instanceof TypeError);
}

r = new RegExp ("a", "mig");
assert (r.toString () == "/a/gim");
try {
  r.toString.call({}, "a");
  assert (false)
}
catch (e)
{
  assert (e instanceof TypeError);
}


/* Test continous calls to the exec method to see how does the match
 * updates the lastIndex propertyand see if the match restarts.
 */
var re = new RegExp("a", "g");
result = re.exec("a");
assert (result.length === 1);
assert (result[0] === "a");
assert (re.lastIndex === 1);

assert (re.exec("a") === null);
assert (re.lastIndex === 0);

result = re.exec("a");
assert (result.length === 1);
assert (result[0] === "a");
assert (re.lastIndex === 1);

var re1 = /foo/gim;
var re2 = /bar/g;

try {
  re2.compile("asd", "ggim");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
  assert (re2 == "/bar/g");
}

try {
  re2.compile("[", undefined);
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
  assert (re2 == "/bar/g");
}

try {
  re2.compile(re1, "im");
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
  assert (re2 == "/bar/g");
}

re2.lastIndex = 2;
re2.compile("asd", "im");

assert (re2 == "/asd/im");
assert (re2.global === false);
assert (re2.ignoreCase === true);
assert (re2.multiline === true);
assert (re2.source === "asd");
assert (re2.lastIndex === 0);

re2.compile(re1);
assert (re2.toString() === re1.toString());
assert (re2.global === re1.global);
assert (re2.ignoreCase === re1.ignoreCase);
assert (re2.multiline === re1.multiline);
assert (re2.source === re1.source);
assert (re2.lastIndex === 0);
