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

var setterCalled = false;

class Base {
  func () {
    return 5;
  }
  funcArrow () {
    return () => 5;
  }
  ["com" + "puted"] () {
    return 10;
  }
  get getter () {
    return 6;
  }
  set setter (a) {
    setterCalled = true;
  }
  getSuperValueOf() {
    return super.valueOf;
  }
}

class Derived extends Base {
  func () {
    return super.func();
  }
  funcArrow () {
    return () => super.func();
  }
  ["com" + "puted"] () {
    return super["com" + "puted"]();
  }
  get getter () {
    return super.getter;
  }
  set setter (a) {
    super.setter = a;
  }
  deleteSuperReference () {
    delete super.a;
  }
}

var derived = new Derived;
var base = new Base;

assert (derived.func() === 5);
assert (derived.funcArrow()() === 5);
assert (derived.computed() === 10);
assert (derived.getter === 6);
derived.setter = 7;
assert (setterCalled === true);
assert (base.getSuperValueOf() === Object.prototype.valueOf);

try {
  derived.deleteSuperReference();
  assert (false);
} catch (e) {
  assert (e instanceof ReferenceError);
}
