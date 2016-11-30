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

// 1.
var simple_obj = {a: 1, b: 2, c: 3, d: 4};
for (var prop_of_simple_obj in simple_obj) {
    simple_obj[prop_of_simple_obj] += 4;
}

assert(simple_obj.a === 5
       && simple_obj.b === 6
       && simple_obj.c === 7
       && simple_obj.d === 8);

// 2.
for
    (
    var
        prop_of_simple_obj in simple_obj
    ) {
    simple_obj[prop_of_simple_obj] -= 4;
}

assert(simple_obj.a === 1
       && simple_obj.b === 2
       && simple_obj.c === 3
       && simple_obj.d === 4);

// 3.
function test() {
  var cnt = 0;

  for (var prop_of_simple_obj in simple_obj) {
    if (prop_of_simple_obj === 'b')
      continue;

    cnt++;

    simple_obj[prop_of_simple_obj] += 4;
  }

  return cnt;
}

var ret_val = test();

assert((simple_obj.a === 5
        && simple_obj.b === 2
        && simple_obj.c === 7
        && simple_obj.d == 8)
       && ret_val === 3);

// 4.
var array_obj = new Array(1, 2, 3, 4, 5, 6, 7);
var prop_of_array_obj;

array_obj.eight = 8;

for (prop_of_array_obj in array_obj) {
    array_obj[prop_of_array_obj] += 1;
}

assert(array_obj[0] === 2
       && array_obj[1] === 3
       && array_obj[2] === 4
       && array_obj[3] === 5
       && array_obj[4] === 6
       && array_obj[5] === 7
       && array_obj[6] === 8
       && array_obj['eight'] === 9);

// 5.
var null_obj = null;
for (var prop_of_null_obj in null_obj) {
    assert(false);
}

// 6.
var empty_object = {};
for (var prop_of_empty_object in empty_object) {
    assert(false);
}

// 7.
for (var i in undefined) {
    assert(false);
}

// 8.
var base_obj = {base_prop: "base"};

function constr() {
    this.derived_prop = "derived";
}

constr.prototype = base_obj;

var derived_obj = new constr();

for (var prop_of_derived_obj in derived_obj) {
    derived_obj[prop_of_derived_obj] += "A";
}

assert(derived_obj.base_prop === "baseA" && derived_obj.derived_prop === "derivedA");

// 9.
log = {};
count = 0;

for (i in {q : 1})
{
  log [i] = true;
  count++;
}

assert (count == 1 && 'q' in log);

// 10.
log = {};
count = 0;

for (i in {q : 1, p : 2, get f() { ; }, set f (v) { ; }, get t () { }, set c (v) {}})
{
  log [i] = true;
  count++;
}

assert (count == 5
        && 'q' in log
        && 'p' in log
        && 'f' in log
        && 't' in log
        && 'c' in log);

// 11.
log = {};
count = 0;

var a = [];
a[5] = 5;
for (var x in a)
{
  log[x] = true;
  count++;
}

assert (count == 1
        && '5' in log);

// 12.
log = {};
count = 0;

q = { c : 3, d : 4 };

function p_constructor ()
{
  this.a = 1;
  this.b = 2;

  return this;
}

p_constructor.prototype = q;
p = new p_constructor ();

Object.defineProperty (p, 'h', { value : 5, enumerable : false, configurable : true });
Object.defineProperty (q, 'h', { value : 6, enumerable : true, configurable : true });

for (var i in p)
{
  log[i] = true;
  count++;
}

assert (count == 4
        && 'a' in log
        && 'b' in log
        && 'c' in log
        && 'd' in log);

// 13.
log = {};
count = 0;

function f()
{
  var tmp = { a: 1, b: 2, c: 3, d: 4 };

  return tmp;
}

for (var i in f())
{
  log[i] = true;
  count++;
}

assert (count == 4
        && 'a' in log
        && 'b' in log
        && 'c' in log
        && 'd' in log);

// 14.
log = {};
count = 0;

b = 'prop';
c = { prop : 1 };
Boolean.prototype.boolean_prototype_prop = 1;

for (a in b in c)
{
  log[a] = true;
  count++;
}

assert (count == 1
        && 'boolean_prototype_prop' in log);

// 15.
log = {};
count = 0;

for (a in 'prop' in { prop : 1 })
{
  log[a] = true;
  count++;
}

assert (count == 1
        && 'boolean_prototype_prop' in log);

// 16.
a = 'str';
b = {};
for ((a in b) ; ; )
{
  break;
}

// 17.
log = {};
count = 0;

var base_obj = { base_prop1: "base1", base_prop2: "base2" };

function l () {
    this.derived_prop1 = "derived1";
    this.derived_prop2 = "derived2";
}

l.prototype = base_obj;

var derived_obj = new l();

for (var prop_of_derived_obj in derived_obj) {
  delete derived_obj.derived_prop1;
  delete derived_obj.derived_prop2;
  delete base_obj.base_prop1;
  delete base_obj.base_prop2;

  log[prop_of_derived_obj] = true;
  count++;
}

assert(count == 1
       && ('base_prop1' in log
           || 'base_prop2' in log
           || 'derived_prop1' in log
           || 'derived_prop2' in log));
