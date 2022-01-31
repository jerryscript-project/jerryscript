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

function assertNameExists(func) {
  assert(func.hasOwnProperty('name') === true);
}
function assertNameNotExists(func) {
  assert(func.hasOwnProperty('name') === false);
  assert(Function.prototype.name === '');
  assert(func.name === '');
}

function assertConfigurableOnlyMethod(func) {
  let desc = Object.getOwnPropertyDescriptor(func, 'name');
  assert(desc.configurable === true);
  assert(desc.writable === false);
  assert(desc.enumerable === false);

  delete func.name;
  assertNameNotExists(func);

  Object.defineProperty(func, 'name', {value: 'newName', configurable: true});
  assert (Object.getOwnPropertyDescriptor(func, 'name').value === 'newName');
  assertNameExists(func);

  delete func.name;
  assertNameNotExists(func);
  Object.defineProperty(func, 'name', desc);
}

function assertConfigurableOnlyAccessor(func, name, method) {
  let accessor = Object.getOwnPropertyDescriptor(func, name)[method];
  assertConfigurableOnlyMethod(accessor);
}

function assertMethodName(func, name, functionName = name) {
  assertNameExists(func);
  assertConfigurableOnlyMethod(func)
  assert(Object.getOwnPropertyDescriptor(func, 'name').value === functionName)
}

function assertGetterName(obj, name, functionName = name) {
  assertConfigurableOnlyAccessor(obj, name, 'get');
  assert(Object.getOwnPropertyDescriptor(obj, name).get['name'] === 'get ' + functionName)
}

function assertSetterName(obj, name, functionName = name) {
  assertConfigurableOnlyAccessor(obj, name, 'set');
  assert(Object.getOwnPropertyDescriptor(obj, name).set['name'] === 'set ' + functionName)
}

var func1 = function () {};
assertMethodName(func1, 'func1');

var func2 = function bar() {};
assertMethodName(func2, 'bar');

var func3 = (function () {}).prototype.constructor;
assert(typeof func3 === 'function');
assertMethodName(func3, '');

var func4;
func4 = function () {}
assertMethodName(func4, 'func4');

var func5;
func5 = function bar () {}
assertMethodName(func5, 'bar');

var func6;
(func6) = function () {}
assertMethodName(func6, '');

var func7;
(func7) = function bar () {}
assertMethodName(func7, 'bar');

let emptySymbolMethod = Symbol();
let namedSymbolMethod = Symbol('foo');
let emptySymbolGetter = Symbol();
let namedSymbolGetter = Symbol('foo');
let emptySymbolSetter = Symbol();
let namedSymbolSetter = Symbol('foo');

var o = {
  func1() {},
  func2: function () {},
  func3: function bar() {},
  func4: () => {},
  func5: class {},
  func6: class A {},
  func7: class name { static name () {} },
  ['func' + '8']() {},
  ['func' + '9']: function () {},
  ['func' + '10']: function bar() {},
  ['func' + '11']: () => {},
  ['func' + '12']: class {},
  ['func' + '13']: class A {},
  ['func' + '14']: class name { static name () {} },
  get func15() {},
  get ['func' + '16']() {},
  set func17(a) {},
  set ['func' + '18'](a) {},
  [emptySymbolMethod]() {},
  [namedSymbolMethod]() {},
  get [emptySymbolGetter]() {},
  get [namedSymbolGetter]() {},
  set [emptySymbolSetter](a) {},
  set [namedSymbolSetter](a) {},
}

assertMethodName(o.func1, 'func1');
assertMethodName(o.func2, 'func2');
assertMethodName(o.func3, 'bar');
assertMethodName(o.func4, 'func4');
assertMethodName(o.func5, 'func5');
assertMethodName(o.func6, 'A');
assert(typeof o.func7 === 'function');

assertMethodName(o.func8, 'func8');
assertMethodName(o.func9, 'func9');
assertMethodName(o.func10, 'bar');
assertMethodName(o.func11, 'func11');
assertMethodName(o.func12, 'func12');
assertMethodName(o.func13, 'A');
assert(typeof o.func14 === 'function');

assertGetterName(o, 'func15');
assertGetterName(o, 'func16');
assertSetterName(o, 'func17');
assertSetterName(o, 'func17');

assertMethodName(o[emptySymbolMethod], '');
assertMethodName(o[namedSymbolMethod], '[foo]');
assertGetterName(o, emptySymbolGetter, '');
assertGetterName(o, namedSymbolGetter, '[foo]');
assertSetterName(o, emptySymbolSetter, '');
assertSetterName(o, namedSymbolSetter, '[foo]');

class A  {
  constructor () {}
  func1() {}
  get func2() {}
  set func3(a) {}

  static func4() {}
  static get func5() {}
  static set func6(a) {}

  ['func' + '7']() {}
  get ['func' + '8']() {}
  set ['func' + '9'](a) {}

  static ['func' + '10']() {}
  static get ['func' + '11']() {}
  static set ['func' + '12'](a) {}

  [emptySymbolMethod]() {}
  [namedSymbolMethod]() {}
  get [emptySymbolGetter]() {}
  get [namedSymbolGetter]() {}
  set [emptySymbolSetter](a) {}
  set [namedSymbolSetter](a) {}

  static [emptySymbolMethod]() {}
  static [namedSymbolMethod]() {}
  static get [emptySymbolGetter]() {}
  static get [namedSymbolGetter]() {}
  static set [emptySymbolSetter](a) {}
  static set [namedSymbolSetter](a) {}
}

assertMethodName(A.prototype.func1, 'func1');
assertGetterName(A.prototype, 'func2');
assertSetterName(A.prototype, 'func3');

assertMethodName(A.func4, 'func4');
assertGetterName(A, 'func5');
assertSetterName(A, 'func6');

assertMethodName(A.prototype.func7, 'func7');
assertGetterName(A.prototype, 'func8');
assertSetterName(A.prototype, 'func9');

assertMethodName(A.func10, 'func10');
assertGetterName(A, 'func11');
assertSetterName(A, 'func12');

assertMethodName(A[emptySymbolMethod], '');
assertMethodName(A[namedSymbolMethod], '[foo]');
assertGetterName(A, emptySymbolGetter, '');
assertGetterName(A, namedSymbolGetter, '[foo]');
assertSetterName(A, emptySymbolSetter, '');
assertSetterName(A, namedSymbolSetter, '[foo]');

assertMethodName(A.prototype[emptySymbolMethod], '');
assertMethodName(A.prototype[namedSymbolMethod], '[foo]');
assertGetterName(A.prototype, emptySymbolGetter, '');
assertGetterName(A.prototype, namedSymbolGetter, '[foo]');
assertSetterName(A.prototype, emptySymbolSetter, '');
assertSetterName(A.prototype, namedSymbolSetter, '[foo]');

class B  {
  func1() {}
  get func2() {}
  set func3(a) {}

  static func4() {}
  static get func5() {}
  static set func6(a) {}

  ['func' + '7']() {}
  get ['func' + '8']() {}
  set ['func' + '9'](a) {}

  static ['func' + '10']() {}
  static get ['func' + '11']() {}
  static set ['func' + '12'](a) {}

  [emptySymbolMethod]() {}
  [namedSymbolMethod]() {}
  get [emptySymbolGetter]() {}
  get [namedSymbolGetter]() {}
  set [emptySymbolSetter](a) {}
  set [namedSymbolSetter](a) {}

  static [emptySymbolMethod]() {}
  static [namedSymbolMethod]() {}
  static get [emptySymbolGetter]() {}
  static get [namedSymbolGetter]() {}
  static set [emptySymbolSetter](a) {}
  static set [namedSymbolSetter](a) {}
}

assertMethodName(B.prototype.func1, 'func1');
assertGetterName(B.prototype, 'func2');
assertSetterName(B.prototype, 'func3');

assertMethodName(B.func4, 'func4');
assertGetterName(B, 'func5');
assertSetterName(B, 'func6');

assertMethodName(B.prototype.func7, 'func7');
assertGetterName(B.prototype, 'func8');
assertSetterName(B.prototype, 'func9');

assertMethodName(B.func10, 'func10');
assertGetterName(B, 'func11');
assertSetterName(B, 'func12');

assertMethodName(B[emptySymbolMethod], '');
assertMethodName(B[namedSymbolMethod], '[foo]');
assertGetterName(B, emptySymbolGetter, '');
assertGetterName(B, namedSymbolGetter, '[foo]');
assertSetterName(B, emptySymbolSetter, '');
assertSetterName(B, namedSymbolSetter, '[foo]');

assertMethodName(B.prototype[emptySymbolMethod], '');
assertMethodName(B.prototype[namedSymbolMethod], '[foo]');
assertGetterName(B.prototype, emptySymbolGetter, '');
assertGetterName(B.prototype, namedSymbolGetter, '[foo]');
assertSetterName(B.prototype, emptySymbolSetter, '');
assertSetterName(B.prototype, namedSymbolSetter, '[foo]');

let names = ['push', 'pop', 'reduce', 'reduceRight'];

for (let n of names) {
  assert(Array.prototype[n].name === n);
}

assert(Array.prototype[Symbol.iterator].name === 'values');
assert(Array.prototype.values.name === 'values');
assert(Object.getOwnPropertyDescriptor(Array, Symbol.species).get.name === 'get [Symbol.species]');
assert(Object.getOwnPropertyDescriptor(String.prototype, Symbol.iterator).value.name === '[Symbol.iterator]');
assert(Object.getOwnPropertyDescriptor(Object.prototype, '__proto__').get.name === 'get __proto__');
assert(Object.getOwnPropertyDescriptor(Object.prototype, '__proto__').set.name === 'set __proto__');

let arFunc;
let array = [];
array['original'] = array;
array['original'][arFunc = ()=>{ }]=function(){}
assertMethodName(array[arFunc], '');

var o = { 0 : class {} };

assertMethodName(o['0'], '0');
