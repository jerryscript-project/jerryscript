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

function check_syntax_error(code)
{
  try {
    eval(code)
    assert(false)
  } catch (e) {
    assert(e instanceof SyntaxError)
  }
}

function check_property(obj, name, value)
{
  property = Object.getOwnPropertyDescriptor(obj, name)
  assert(typeof property === "object")
  assert(property.value === value)
}

check_syntax_error("class C { get a = 5 }");
check_syntax_error("class C { id1 id2 }");
check_syntax_error("class C { a = 5,6 }");
check_syntax_error("class C { set\na = 6 }");
check_syntax_error("class C { constructor }");
check_syntax_error("class C { static constructor }");
check_syntax_error("class C { constructor = 1 }");
check_syntax_error("class C { static constructor = 1 }");
check_syntax_error("class C { f = arguments }");
check_syntax_error("class C { static f = a\\u0072guments }");
check_syntax_error("class C { f = () => arguments }");
check_syntax_error("class C { f = arguments => 1 }");
check_syntax_error("class C { f = ([arguments]) => 1 }");
check_syntax_error("new class { f = eval('arguments') }");
check_syntax_error("new class { f = eval('arguments => 1') }");

var res = 10
var counter = 0

function f1() {
  counter++
  return 5
}

var C1 = class {
  get = "a" + f1()
  static; set; a = () => Math.cos(0)
  v\u0061r
  f\u006fr = () => this
  arguments = this
}

res = new C1
check_property(res, "get", "a5")
check_property(res, "static", undefined)
check_property(res, "set", undefined)
assert(res.a() === 1)
check_property(res, "var", undefined)
assert(res.for() === res)
assert(res.arguments === res)

class C2 {
  constructor(a = this.x, b = this.y) {
    assert(a === undefined)
    assert(b === undefined)
    check_property(this, 'x', 11)
    check_property(this, 'y', "ab")
  }
  x = 5 + 6
  y = "a" + 'b'
}

res = new C2

class C3 {
  constructor() {
    assert(this.x === 1)
    return { z:"zz" }
  }
  x = 1
}

class C4 extends C3 {
  constructor() {
    super()
    assert(Object.getOwnPropertyDescriptor(this, "x") === undefined)
    check_property(this, "y", 2)
    check_property(this, "z", "zz")
  }
  y = 2
}
new C4

var o = {}
class C5 extends C3 {
  'pr op' = o
  3 = true
}
res = new C5
assert(Object.getOwnPropertyDescriptor(res, "x") === undefined)
check_property(res, "pr op", o)
check_property(res, "3", true)
check_property(res, "z", "zz")

class C6 {
  a= () => this
  b= this
}

class C7 extends C6 {
  c= () => this
  d= this
}

count = 0
class C8 extends C7 {
  constructor() {
    count++
    super()
  }

  e= () => this
  f= this
}

var res = new C8
assert(res.a() === res)
assert(res.b === res)
assert(res.c() === res)
assert(res.d === res)
assert(res.e() === res)
assert(res.f === res)

count = 0
class C9 {
  a=assert(++count === 5)
  a=assert(++count === 6)
  a=assert(++count === 7)
  a=assert(++count === 8)
  static a=assert(++count === 1)
  static a=assert(++count === 2)
  static a=assert(++count === 3)
  static a=assert(++count === 4)
}

assert(count === 4)
new C9
assert(count === 8)

count = 0
class C10 {
  [(assert(++count == 1), "aa")] = assert(++count == 5);
  [(assert(++count == 2), "bb")] = assert(++count == 6);
  cc = assert(++count == 7);
  [(assert(++count == 3), "aa")] = assert(++count == 8);
  [(assert(++count == 4), "bb")] = assert(++count == 9);
}

assert(count == 4)
assert(Reflect.ownKeys(new C10).toString() === "aa,bb,cc");
assert(count == 9)

res = "p"
class C11 {
  p1 = assert(Reflect.ownKeys(this).toString() === "");
  [res + 2] = assert(Reflect.ownKeys(this).toString() === "p1");
  [res + 1] = assert(Reflect.ownKeys(this).toString() === "p1,p2");
  p3 = assert(Reflect.ownKeys(this).toString() === "p1,p2");
  [res + 4] = assert(Reflect.ownKeys(this).toString() === "p1,p2,p3");
}
new C11
