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

assert (Math.cos.toString() === "function () { [native code] }");

var has_toString = none.toString() != "function () { /* ecmascript */ }"

function check(f, expected)
{
  assert (f.toString() === (has_toString ? expected : "function () { /* ecmascript */ }"))
}

function none() { return 1; }
check (none, "function none() { return 1; }")
assert (none.bind({}).toString() === "function () { [native code] }")

function single(b) { return 1; }
check (single, "function single(b) { return 1; }")
assert (single.bind({}).toString() === "function () { [native code] }")

function multiple(a,b) { return 1; }
check (multiple, "function multiple(a,b) { return 1; }")
assert (multiple.bind({}).toString() === "function () { [native code] }")

function lots(a,b,c,d,e,f,g,h,i,j,k) { return 1; }
check (lots, "function lots(a,b,c,d,e,f,g,h,i,j,k) { return 1; }")
assert (lots.bind({}).toString() === "function () { [native code] }")
function check() {}

if (check.toString() !== "function () { /* ecmascript */ }")
{
  var a, b, o, c, f;

  /* Function statements. */

  a = 6; function f1(a,b=1,c) { return a /* comment */ + b + c } b = 7
  assert(f1.toString() === "function f1(a,b=1,c) { return a /* comment */ + b + c }")

  a = 6; function * f2(a,b,c=1) {
    function x() {}
  } b = 7
  assert(f2.toString() === "function * f2(a,b,c=1) {\n    function x() {}\n  }")

  a = 6;async  function   f3 ( a , b , c ) { } b = 7
  assert(f3.toString() === "async  function   f3 ( a , b , c ) { }")

  a = 6;async/**/function*f4(a,b,c){} b = 7
  assert(f4.toString() === "async/**/function*f4(a,b,c){}")

  /* Object initializers. */

  o = {f(a) { return a }}
  assert(o.f.toString() === "f(a) { return a }")

  o = {f:function(a){/**/}}
  assert(o.f.toString() === "function(a){/**/}")

  o = { [function(){ return 'f' }()] (a = function() {}) {} }
  assert(o.f.toString() === "[function(){ return 'f' }()] (a = function() {}) {}")

  o = {*  f(a) {}}
  assert(o.f.toString() === "*  f(a) {}")

  o = {*[function(){ return 'f' }()](a) {}}
  assert(o.f.toString() === "*[function(){ return 'f' }()](a) {}")

  o = {async/**/f(a) {}}
  assert(o.f.toString() === "async/**/f(a) {}")

  o = {/**/async   [function(){ return 'f' }()](a) {}/**/}
  assert(o.f.toString() === "async   [function(){ return 'f' }()](a) {}")

  o = {a:1,async/**/*/**/f(a) {},b:1}
  assert(o.f.toString() === "async/**/*/**/f(a) {}")

  o = {async *//
       [function(){ return 'f' }()](a) {}/**/}
  assert(o.f.toString() === "async *//\n       [function(){ return 'f' }()](a) {}")

  o = { get a() {return/**/6} }
  assert(Object.getOwnPropertyDescriptor(o, "a").get.toString() === "get a() {return/**/6}")

  o = { get[function(){ return 'a' }()](){} }
  assert(Object.getOwnPropertyDescriptor(o, "a").get.toString() === "get[function(){ return 'a' }()](){}")

  o = { set a(v) {/**/} }
  assert(Object.getOwnPropertyDescriptor(o, "a").set.toString() === "set a(v) {/**/}")

  o = {/**/set/**/[function(){ return 'a' }()]/**/(v) {/**/}/**/}
  assert(Object.getOwnPropertyDescriptor(o, "a").set.toString() === "set/**/[function(){ return 'a' }()]/**/(v) {/**/}")

  /* Class static functions. */

  c = class { static/**/f() {}/**/ }
  assert(c.f.toString() === "f() {}")

  c = class { static[function(){ return 'f' }()]() {} }
  assert(c.f.toString() === "[function(){ return 'f' }()]() {}")

  c = class { static    *f() {} }
  assert(c.f.toString() === "*f() {}")

  c = class {/**/static *  [function(){ return 'f' }()](a=6){}/**/}
  assert(c.f.toString() === "*  [function(){ return 'f' }()](a=6){}")

  c = class { static/**/async f() {} }
  assert(c.f.toString() === "async f() {}")

  c = class {static   async[function(){ return 'f' }()]() {/**/}}
  assert(c.f.toString() === "async[function(){ return 'f' }()]() {/**/}")

  c = class { static async*f() {}}
  assert(c.f.toString() === "async*f() {}")

  c = class { static async*/**/[function(){ return 'f' }()](){} }
  assert(c.f.toString() === "async*/**/[function(){ return 'f' }()](){}")

  c = class { static/**/get/**/a() {}}
  assert(Object.getOwnPropertyDescriptor(c, "a").get.toString() === "get/**/a() {}")

  c = class {static  set  a(v){}}
  assert(Object.getOwnPropertyDescriptor(c, "a").set.toString() === "set  a(v){}")

  c = class { static   get[function(){ return 'a' }()](){} }
  assert(Object.getOwnPropertyDescriptor(c, "a").get.toString() === "get[function(){ return 'a' }()](){}")

  c = class { static   set[function(){ return 'a' }()](v){}//
            }
  assert(Object.getOwnPropertyDescriptor(c, "a").set.toString() === "set[function(){ return 'a' }()](v){}")

  /* Class functions. */

  o = Object.getPrototypeOf(new class {/**/f() {}/**/})
  assert(o.f.toString() === "f() {}")

  o = Object.getPrototypeOf(new class {[function(){ return 'f' }()](){}})
  assert(o.f.toString() === "[function(){ return 'f' }()](){}")

  o = Object.getPrototypeOf(new class {  * /**/  f() {}  })
  assert(o.f.toString() === "* /**/  f() {}")

  o = Object.getPrototypeOf(new class {  *  [function(){ return 'f' }()]/**/(){}  })
  assert(o.f.toString() === "*  [function(){ return 'f' }()]/**/(){}")

  o = Object.getPrototypeOf(new class {async f() {}})
  assert(o.f.toString() === "async f() {}")

  o = Object.getPrototypeOf(new class {async[function(){ return 'f' }()](){}})
  assert(o.f.toString() === "async[function(){ return 'f' }()](){}")

  o = Object.getPrototypeOf(new class {/**/get/**/a() {}/**/})
  assert(Object.getOwnPropertyDescriptor(o, "a").get.toString() === "get/**/a() {}")

  o = Object.getPrototypeOf(new class { set  a(v){} })
  assert(Object.getOwnPropertyDescriptor(o, "a").set.toString() === "set  a(v){}")

  o = Object.getPrototypeOf(new class {/**/get/**/[function(){ return 'a' }()]() {}/**/})
  assert(Object.getOwnPropertyDescriptor(o, "a").get.toString() === "get/**/[function(){ return 'a' }()]() {}")

  o = Object.getPrototypeOf(new class {  set/**/[function(){ return 'a' }()]( v ){}  })
  assert(Object.getOwnPropertyDescriptor(o, "a").set.toString() === "set/**/[function(){ return 'a' }()]( v ){}")

  /* Function creators. */
  f = Function("a,b", "return a + b")
  assert(f.toString() === "function anonymous(a,b\n) {\nreturn a + b\n}")

  f = Function("", "")
  assert(f.toString() === "function anonymous(\n) {\n\n}")

  f = function*(){}.constructor("a,b", "c", "yield a; return b + c")
  assert(f.toString() === "function* anonymous(a,b,c\n) {\nyield a; return b + c\n}")

  f = async function(){}.constructor("a", "return a + 'x'")
  assert(f.toString() === "async function anonymous(a\n) {\nreturn a + 'x'\n}")

  f = async function*(){}.constructor("a=3", "return a")
  assert(f.toString() === "async function* anonymous(a=3\n) {\nreturn a\n}")

  f = Function("a = function(x) { return x + 3 }", "return a")
  assert(f().toString() === "function(x) { return x + 3 }")

  f = Function("return function(x) { return x + 3 }")
  assert(f().toString() === "function(x) { return x + 3 }")

  /* Arrow functions. */
  f = x => x + 1/**/
  assert(f.toString() === "x => x + 1")

  f =x => { return x + 1 }/**/
  assert(f.toString() === "x => { return x + 1 }")

  f = (/**/) => 'x' //
    + y
  assert(f.toString() === "(/**/) => 'x' //\n    + y")

  f =  async  x  =>  x + 1
  assert(f.toString() === "async  x  =>  x + 1")

  f =/**/async (x,/**/y)=>/**/null
  assert(f.toString() === "async (x,/**/y)=>/**/null")
}
