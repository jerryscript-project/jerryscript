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

function expectSyntaxError(str) {
  try {
    eval(str);
    assert(false);
  } catch (e) {
    assert(e instanceof SyntaxError);
  }
}

function expectTypeError(cb) {
  try {
    cb();
    assert(false);
  } catch (e) {
    assert(e instanceof TypeError);
  }
}

expectSyntaxError("this?.a``");
expectSyntaxError("this?.['a']``");
expectSyntaxError("this?.``");
expectSyntaxError("this.a?.``");
expectSyntaxError("this['a']?.``");
expectSyntaxError("this?.a = 9");
expectSyntaxError("this.a.a.a.a.a?.a = 9");
expectSyntaxError("this?.a.a.a.a.a.a = 9");
expectSyntaxError("this?.a++");
expectSyntaxError("this?.a.a.a.a.a.a++");
expectSyntaxError("this.a.a.a.a.?a.a++");
expectSyntaxError("this?.a--");
expectSyntaxError("this?.a.a.a.a.a.a--");
expectSyntaxError("this.a.a.a.a.?a.a--");

var o = {
  a: 4.1,
  b() {
    return 4.2;
  },
  c: {
    a: 4.3,
    b() {
      return 4.4
    }
  },
  e(...args) {
    return args.reduce((p, c) => p + c);
  }
}


assert(o?.a === 4.1);
assert(o?.a2 === undefined);
assert(this.o?.a === 4.1);
assert(this.o?.a2 === undefined);
assert(this.o?.['a'] === 4.1);
assert(this.o?.['a2'] === undefined);
assert(typeof o?.a === 'number');
assert(typeof o?.a2 === 'undefined');

assert(o?.c?.a === 4.3);
assert(o?.c?.a2 === undefined);
assert(this.o?.c?.a === 4.3);
assert(this.o?.c?.a2 === undefined);
assert(this.o?.c?.['a'] === 4.3);
assert(this.o?.c?.['a2'] === undefined);
assert(typeof o?.c?.a === 'number');
assert(typeof o?.c?.a2 === 'undefined');

assert(o?.d === undefined);
assert(o?.d?.d === undefined);
assert(o?.d?.d?.['d'] === undefined);

assert(o?.b?.() === 4.2);
assert(o?.b2?.() === undefined);

assert(o?.c?.b?.() === 4.4);
assert(o?.c?.b2?.() === undefined);

assert(o?.e(...[1.25, 2.25, 3.25]) === 6.75);
assert(o?.e(...[1.25, 2.25, 3.25], ...[0.25]) === 7);
assert(o?.['e'](...[1.25, 2.25, 3.25]) === 6.75);
assert(o?.['e'](...[1.25, 2.25, 3.25], ...[0.25]) === 7);

assert(o?.e?.(...[1.25, 2.25, 3.25]) === 6.75);
assert(o?.e?.(...[1.25, 2.25, 3.25], ...[0.25]) === 7);
assert(o?.['e']?.(...[1.25, 2.25, 3.25]) === 6.75);
assert(o?.['e']?.(...[1.25, 2.25, 3.25], ...[0.25]) === 7);

// Test short circuit
let count = 0;
assert(undefined?.[count++] === undefined);
assert(undefined?.[count++]() === undefined);
assert(undefined?.[count++]()() === undefined);
assert(null?.[count++] === undefined);
assert(null?.[count++]() === undefined);
assert(null?.[count++]()() === undefined);
assert(count === 0);

// Test optional call
var g = undefined;

function f () {
  return 4.5;
}

assert(g?.() === undefined);
assert(this?.g?.() === undefined);
assert(this.g?.() === undefined);

expectTypeError(_ => {
  this.g();
});

assert(f?.() === 4.5);
assert(this?.f?.() === 4.5);
assert(this.f?.() === 4.5);
assert(f() === 4.5);

// test direct eval
var a = 5.1;
eval('a = 5.2');
assert(a === 5.2);
eval?.('a = 5.3');
assert(a === 5.3);
eval?.(...['a = 5.4']);
assert(a === 5.4);

const saved_eval = eval;
eval = undefined;

eval?.('a = 5.5')
assert(a === 5.4);
eval?.(...['a = 5.5'])
assert(a === 5.4);

eval = saved_eval;

// test optional private property access
class A {
  #a = 5.1;

  test(o) {
    return o?.#a;
  }
}

let instance = new A;
assert(instance.test(instance) === 5.1);
assert(instance.test(undefined) === undefined);
assert(instance.test(null) === undefined);
expectTypeError(_ => {
  instance.test({});
});
