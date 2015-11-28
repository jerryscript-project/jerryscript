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

var t;

t = /\//.exec("/");
assert (t == "/");

t = /[/]/.exec("/");
assert ("a"+/x/+"b" == "a/x/b");

t = /\/\[[\]/]/.exec("/[/");
assert (t == "/[/");

t = /\u0000/.exec("\u0000");
assert (t == "\u0000");

try {
  eval("/" + String.fromCharCode("0x0000") + "/");
} catch (e) {
  assert (false);
}

try {
  eval("var x = 5\n\n/foo/");
  assert(false);
} catch (e) {
  assert(e instanceof SyntaxError);
}

try {
  eval("var x = 5;\n\n/foo/");
} catch (e) {
  assert(false);
}

try {
  eval("for (;false;/abc/.exec(\"abc\")) {5}");
} catch (e) {
  assert(false);
}

try {
  eval("var a = [] /foo/");
  assert(false);
} catch (e) {
  assert(e instanceof SyntaxError);
}

try {
  eval("/");
  assert(false);
} catch (e) {
  assert(e instanceof SyntaxError);
}

try {
  eval("var x = /aaa/");
} catch (e) {
  assert (false);
}

try {
  eval("{}/a/g");
} catch (e) {
  assert (false);
}

try {
  eval("var a, g; +{}/a/g");
} catch (e) {
  assert (false);
}
