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

var setter1_called = false;
var setter2_called = false;

var obj1 = {
  method1() {
    return 1;
  },
  ['method2']() {
    return 2;
  },
  *method3() {
    return 3;
  },
  *['method4']() {
    return 4;
  },
  get getter1() {
    return 5;
  },
  get ['getter2']() {
    return 6;
  },
  set setter1(rhs) {
    setter1_called = true;
    assert(rhs === 7);
  },
  set ['setter2'](rhs) {
    setter2_called = true;
    assert(rhs === 8);
  },
}

var obj2 = {
  method1() {
    return super.method1();
  },
  ['method2']() {
    return super.method2();
  },
  *method3() {
    return super.method3();
  },
  *['method4']() {
    return super.method4();
  },
  get getter1() {
    return super.getter1;
  },
  get ['getter2']() {
    return super.getter2;
  },
  set setter1(rhs) {
    super.setter1 = rhs;
  },
  set ['setter2'](rhs) {
    super.setter2 = rhs;
  },
  __proto__: obj1,
}

assert(obj2.method1() === 1);
assert(obj2.method2() === 2);
assert(obj2.method3().next().value.next().value === 3);
assert(obj2.method4().next().value.next().value === 4);

assert(obj2.getter1 === 5);
assert(obj2.getter2 === 6);

obj2.setter1 = 7;
assert(setter1_called);
obj2.setter2 = 8;
assert(setter2_called);

let obj3 = {
  a() {
    return 9;
  }
}

let obj4 = {
  a() {
    return eval('super.a()');
  },
  __proto__: obj3,
}

assert(obj4.a() === 9);

let obj5 = {
  a() {
    return (_ => super.a())();
  },
  __proto__: obj3,
}

assert(obj5.a() === 9);

let obj6 = {
  a() {
    return (_ => _ => super.a())()();
  },
  __proto__: obj3,
}

assert(obj6.a() === 9);


function checkSyntax(src) {
  try {
    eval(src);
    assert(false);
  } catch (e) {
    assert(e instanceof SyntaxError);
  }
}

checkSyntax('({ a : function () { super.a }})')
checkSyntax('({ [\'a\'] : function () { super.a }})')
