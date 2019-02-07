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

/* Test object literals. */

var local = 0;

function f(x)
{
  return x + "et";
}

var o = {
  a: 5,
  [
    "pr" +
    "op"]  : 6,

  [f
   ("g")
   ]
   : 7,

  [f(
   "s"
   )  ]: 8,

  get    [f
    ("res")
    ]
    () { return 9 },

  set
  [f("res")](
   value) { local = value },
};

assert(o.a === 5);
assert(o.prop === 6);
assert(o.get === 7);
assert(o.set === 8);

local = 0;
o.reset = 10;
assert(local === 10);
assert(o.reset === 9);

/* Test classes. */

function fxy() {
  return "xy";
}

class C {
  [fxy()] () {
    return 6;
  }

  static [fxy()]() {
    return 7;
  }

  get ["a" + 1]() {
    return 8;
  }

  set ["a" + 1](x) {
    local = x;
  }

  static get ["a" + 1]() {
    return 10;
  }

  static set ["a" + 1](x) {
    local = x;
  }
};

var c = new C;
assert(c.xy() === 6);
assert(C.xy() === 7);

local = 0;
c.a1 = 9;
assert(local === 9);
assert(c.a1 === 8);

local = 0;
C.a1 = 11;
assert(local === 11);
assert(C.a1 === 10);

class D {
  [(() => "const" + "ructor")()] (arg) {
    this.a = arg;
  }
}

var d = new D;
assert(d.a === undefined);
d.constructor(7);
assert(d.a === 7);

class E {
  get ["_constructor_".substring(1,12)]() {
    return this.a;
  }
}

var e = new E;
assert(e.constructor === undefined);
e.a = 8;
assert(e.constructor === 8);

function throw_error(snippet)
{
  try {
    eval(snippet);
    assert(false);
  } catch (e) {
    assert(e instanceof TypeError);
  }
}

throw_error("new class { static ['proto' + 'type'] () {} }");
throw_error("new class { static get ['proto' + 'type'] () {} }");
throw_error("new class { static set ['proto' + 'type'] (x) {} }");
