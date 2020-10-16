/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

function must_throw(str) {
  try {
    eval("switch (1) { default: " + str + "}");
    assert(false);
  } catch (e) {
  }

  try {
    eval(str);
    assert(false);
  }
  catch (e) {
  }

  must_throw_strict(str);
}

function must_throw_strict(str) {
  try {
    eval ("'use strict'; switch (1) { default: " + str + "}");
    assert (false);
  } catch (e) {
  }

  try {
    eval("'use strict'; " + str);
    assert(false);
  } catch (e) {
  }
}

must_throw("class {}");
must_throw("class class {}");
must_throw("class A { constructor() {} this.a = 5 }");
must_throw("class A { constructor() {} constructor() {} }");
must_throw("class A { static prototype() {} }");
must_throw("class A { static get prototype() {} }");
must_throw("class A { static set prototype() {} }");
must_throw("class A { static prototyp\u{0065}() {} }");
must_throw("class A { static get prototyp\u{0065}() {} }");
must_throw("class A { static set prototyp\u{0065}() {} }");
must_throw("class A { static 'prototype'() {} }");
must_throw("class A { static get 'prototype'() {} }");
must_throw("class A { static set 'prototype'() {} }");
must_throw("class A { static 'prototyp\u{0065}'() {} }");
must_throw("class A { static get 'prototyp\u{0065}'() {} }");
must_throw("class A { static set 'prototyp\u{0065}'() {} }");
must_throw("class A { get constructor() {} }");
must_throw("class A { set constructor() {} }");
must_throw("class A {}; A()");
must_throw("class X {}; var o = {}; Object.defineProperty(o, 'p', { get: X, set: X }); o.p;");
must_throw("var a = new A; class A {};");
must_throw("class A { g\\u0065t e() {} }");
must_throw('class A { "static" e() {} }');
must_throw('class A { *constructor() {} }');

assert(eval("class A {}") === undefined);
assert(eval("var a = class A {}") === undefined);
assert(eval("var a = class {}") === undefined);
assert(eval("class A { ; ; ; ;;;;;;;;;;;; ; ; ;;;;;;;;;;;;;;;;;;;;;;;;; }") === undefined);
assert(eval('class A {"constructor"() {} }') === undefined);
assert(isNaN (eval('switch(1) { default: (class A{} % 1) }')));

class A1 {
  ["constructor"]() {
    return 5;
  }
}

assert ((new A1).constructor() === 5);

class A2 {
  *["constructor"]() {
    yield 5;
  }
}

assert ((new A2).constructor().next().value === 5);

class B {
}

var b = new B;
assert (typeof B  === "function");
assert (typeof b === "object");
assert (b.constructor === B);

class C {
  c1() {
    return 5;
  }

  c2() {
    return this._c;
  }
  3() {
    return 3;
  }

  super() {
    return 42;
  }
  return() {
    return 43;
  }

  static *constructor() {
    return 44;
  }
}

var c = new C;
assert (c.c1() === 5);
assert (c.c2() === undefined);
assert (c["3"]() === 3);
assert (c.super() === 42);
assert (c.return() === 43);
assert (c.constructor === C);
assert (C.constructor().next().value === 44);

class D {
  constructor(d) {
    this._d = d;
  }

  d1() {
    return this._d;
  }
}
var d = new D(5);
assert (d.d1() === 5);
assert (d.constructor === D);

class E {
  constructor(e) {
    this._e = e;
  }

  get e() {
    return this._e;
  }

  set e(e) {
    this._e = e;
  }

  get () {
    return 11;
  }

  set () {
    return 12;
  }
}
var e = new E (5);
assert (e.e === 5);
e.e = 10;
assert (e.e === 10);
assert (e.get() === 11);
assert (e.set() === 12);
assert (e.constructor === E);

var F = class ClassF {
  constructor(f) {
    this._f = f;
  }

  static f1() {
    return this;
  }

  static f2() {
    return this._f;
  }

  static f3(a, b) {
    return a + b;
  }

  static constructor(a) {
    return a;
  }

  static static(a) {
    return a;
  }

  static 2 (a) {
    return 2 * a;
  }

  static function(a) {
    return 3 * a;
  }
}

var f = new F(5);

assert (f.f1 === undefined);
assert (f.f2 === undefined);
assert (F.f1() === F);
assert (F.f2() === undefined);
assert (F.f3(1, 1) === 2);
assert (F.constructor(5) === 5);
assert (F.static(5) === 5);
assert (F["2"](5) === 10);
assert (F.function(5) === 15);
assert (f.constructor === F);

var G = class {
  static set a(a) {
    this._a = a;
  }
  static get a() {
    return this._a;
  }
  static set 1(a) {
    this._a = a;
  }
  static get 1() {
    return this._a;
  }

  static get() {
    return 11;
  }

  static set() {
    return 12;
  }

  static set constructor(a) {
    this._a = a;
  }
  static get constructor() {
    return this._a;
  }

  static g1() {
    return 5;
  }

  static g1() {
    return 10;
  }
}

G.a = 10;
assert (G.a === 10);
assert (G.g1() === 10);
G["1"] = 20;
assert (G["1"] === 20);
assert (G.get() == 11);
assert (G.set() == 12);
G.constructor = 30;
assert (G.constructor === 30);

class H {
  method() { assert (typeof H === 'function'); return H; }
}

let H_original = H;
var H_method = H.prototype.method;
C = undefined;
assert(C === undefined);
C = H_method();
assert(C === H_original);

var I = class C {
  method() { assert(typeof C === 'function'); return C; }
}

let I_original = I;
var I_method = I.prototype.method;
I = undefined;
assert(I === undefined);
I = I_method();
assert(I == I_original);

var J_method;
class J {
  static [(J_method = eval('(function() { return J; })'), "X")]() {}
}
var J_original = J;
J = 6;
assert (J_method() == J_original);

var K_method;
class K {
  constructor () {
    K_method = function() { return K; }
  }
}
var K_original = K;
new K;
K = 6;
assert (K_method() == K_original);

var L_method;
class L extends (L_method = function() { return L; }) {
}
var L_original = L;
L = 6;
assert (L_method() == L_original);

/* Test cleanup class environment */
try {
  class A {
    [d]() {}
  }
  let d;
  assert(false);
} catch (e) {
  assert(e instanceof ReferenceError);
}

try {
  class A extends d {}
  let d;
  assert(false);
} catch (e) {
  assert(e instanceof ReferenceError);
}
try {
  var a = 1 + 2 * 3 >> class A extends d {};
  let d;
  assert(false);
} catch (e) {
  assert(e instanceof ReferenceError);
}
