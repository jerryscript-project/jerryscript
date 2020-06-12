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


// Test %GeneratorPrototype%
(function () {
  function* generatorFn(){}
  var ownProto = Object.getPrototypeOf(generatorFn());
  var sharedProto = Object.getPrototypeOf(ownProto);

  assert(ownProto === generatorFn.prototype);
  assert(sharedProto !== Object.prototype);
  assert(sharedProto === Object.getPrototypeOf(function*(){}.prototype));
  assert(sharedProto.hasOwnProperty('next'));
})();

// Test %GeneratorPrototype% prototype chain
(function () {
  function* generatorFn(){}
  var g = generatorFn();
  var ownProto = Object.getPrototypeOf(g);
  var sharedProto = Object.getPrototypeOf(ownProto);
  var iterProto = Object.getPrototypeOf(sharedProto);

  assert(ownProto === generatorFn.prototype);
  assert(iterProto.hasOwnProperty(Symbol.iterator));
  assert(!sharedProto.hasOwnProperty(Symbol.iterator));
  assert(!ownProto.hasOwnProperty(Symbol.iterator));
  assert(g[Symbol.iterator]() === g);
})();

// Test %GeneratorPrototype% prototype chain
(function () {
  function* g(){}
  var iterator = new g.constructor("a","b","c","() => yield\n yield a; yield b; yield c;")(1,2,3);

  var item = iterator.next();
  assert(item.value === 1);
  assert(item.done === false);

  item = iterator.next();
  assert(item.value === 2);
  assert(item.done === false);

  item = iterator.next();
  assert(item.value === 3);
  assert(item.done === false);

  item = iterator.next();
  assert(item.value === undefined);
  assert(item.done === true);

  assert(g.constructor === (function*(){}).constructor);
})();

// Test GeneratorFunction parsing
(function () {
  function *f() {};

  try {
    Object.getPrototypeOf(f).constructor("yield", "");
  } catch (e) {
    assert(e instanceof SyntaxError);
  }
})();

// Test correct property membership
(function () {
  function *f() {};

  Object.getPrototypeOf(f).xx = 5;
  assert(Object.getPrototypeOf(f).prototype.constructor.xx === 5);
})();

// Test GetPrototypeFromConstructor
(function () {
  function *f() {};

  var originalProto = f.prototype;
  f.prototype = 5;
  assert(Object.getPrototypeOf(f()) === Object.getPrototypeOf(originalProto));
  var fakeProto = { x : 6 };
  f.prototype = fakeProto;
  assert(Object.getPrototypeOf(f()) === fakeProto);
  assert(f.next === undefined);
})();
