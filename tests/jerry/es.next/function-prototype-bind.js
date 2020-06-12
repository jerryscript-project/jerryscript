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

/* extended class */
(function() {
  class C extends Function {}
  var c = new C("x", "y", "return this.foo + x + y;").bind({foo : 1}, 2);
  assert(c(3) === 6);
  assert(c instanceof C);
})();

function boundPrototypeChecker(f, proto) {
  Object.setPrototypeOf(f, proto);

  var boundFunc = Function.prototype.bind.call(f, null);
  assert(Object.getPrototypeOf(boundFunc) === proto);
}

/* generator function */
(function() {
  var f = function*(){};
  boundPrototypeChecker(f, Function.prototype)
  boundPrototypeChecker(f, {})
  boundPrototypeChecker(f, null);
})();

/* arrow function */
(function() {
  var f = () => 5;
  boundPrototypeChecker(f, Function.prototype)
  boundPrototypeChecker(f, {})
  boundPrototypeChecker(f, null);
})();

/* simple class */
(function() {
  class C {};
  boundPrototypeChecker(C, Function.prototype)
  boundPrototypeChecker(C, {})
  boundPrototypeChecker(C, null);
})();

/* subclasses */
(function() {
  function boundPrototypeChecker(superclass) {
    class C extends superclass {
      constructor() {
        return Object.create(null);
      }
    }
    var boundF = Function.prototype.bind.call(C, null);
    assert(Object.getPrototypeOf(boundF) === Object.getPrototypeOf(C));
  }

  boundPrototypeChecker(function(){});
  boundPrototypeChecker(Array);
  boundPrototypeChecker(null);
})();
