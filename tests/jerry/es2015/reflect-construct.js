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

try {
  Reflect.construct ();
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  Reflect.construct (Date);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

var d = Reflect.construct (Date, [1776, 6, 4]);
assert (d instanceof Date);
assert (d.getFullYear () === 1776);

function func1 (a, b, c) {
  this.sum = a + b + c;
}

var args = [1, 2, 3];
var object1 = new func1 (...args);
var object2 = Reflect.construct (func1, args);

assert (object2.sum === 6);
assert (object1.sum === 6);

function CatClass () {
  this.name = 'Cat';
}

function DogClass () {
  this.name = 'Dog';
}

var obj1 = Reflect.construct (CatClass, args, DogClass);
assert (obj1.name === 'Cat');
assert (!(obj1 instanceof CatClass));
assert (obj1 instanceof DogClass);

try {
  Reflect.construct (func1, 5, 5);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  Reflect.construct (5, 5);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

function func2 () {
  throw 5;
}

try {
  Reflect.construct (func2, {});
  assert (false);
} catch (e) {
  assert (e === 5);
}
