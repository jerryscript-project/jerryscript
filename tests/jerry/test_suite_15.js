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

(function tc_15_02_01__004() {
  var a = 1;
  assert(typeof Object(a) === 'object');
})();

(function tc_15_02_01__005() {
  var a = true;

  assert(typeof Object(a) === 'object');
})();

(function tc_15_02_01__002() {
  var a;
  a = Object(null);

  assert(typeof a === 'object');
})();

(function tc_15_02_01__003() {
  var a;
  a = Object(undefined);

  assert(typeof a === 'object');
})();

(function tc_15_02_01__006() {
  var a = false;
  assert(typeof Object(a) === 'object');
})();

(function tc_15_02_01__007() {
  var a = "some string";

  assert(typeof Object(a) === 'object');
})();

(function tc_15_02_01__009() {
  var a = new Number(123.5);
  assert (typeof Object(a) === 'object');
})();

(function tc_15_02_01__008() {
  var a = "some string";
  assert(typeof Object(a) === 'object');
})();

(function tc_15_02_01__010() {
  var a = new String("nice string");
  assert(typeof Object(a) === 'object');
})();

(function tc_15_02_01__001() {
  var a;
  a = Object();

  assert(typeof a === 'object');
})();

(function tc_15_02_03__018() {
  var object = {
    prop1: Number,
    prop2: String,
    prop3: Boolean
  }

  Object.seal(object);

  check = delete object.prop1 || delete object.prop2 || delete object.prop3;

  object.prop4 = 3;

  check = check || Boolean(object.prop4);

  assert(!check);
})();

(function tc_15_02_03__017() {
  var object = {
    prop1: "qwerty",
    prop2: 123,
  }

  Object.freeze(object);

  object.prop1 = "asdf";
  object.prop3 = true;

  assert(!(object.prop1 === "asdf") && !delete object.prop2 && !object.prop3);
})();

(function tc_15_02_03__001() {
  assert(typeof Object.getPrototypeOf(Object) == "function" &&
          Object.length == 1);
})();

(function tc_15_02_03__004() {
  var object = {
    prop1: Number,
    prop2: String,
    prop3: true,
    prop4: 0
  }

  var keys = Object.keys(object);

  assert(keys[0] == "prop1" &&
          keys[1] == "prop2" &&
          keys[2] == "prop3" &&
          keys[3] == "prop4");

})();

(function tc_15_02_03__019() {
  var emptyObject = {}

  var properties = {
    prop1: {
      writable: true,
      enumerable: true,
      configurable: false,
      value: "I'm prop1"
    },
    prop2: {
      writable: true,
      enumerable: true,
      configurable: false,
      value: "I'm prop2"
    }
  }

  var isEnumerable = true;
  var isConfigurable = true;
  var isWritable = false;

  Object.defineProperties(emptyObject, properties);

  emptyObject.prop1 = "hello";
  emptyObject.prop2 = "world";

  if (emptyObject.prop1 === "hello" && emptyObject.prop2 == "world")
    isWritable = true;

  for (p in emptyObject) {
    if (emptyObject[p] === "hello")
      isEnumerable = !isEnumerable;
    else if (emptyObject[p] === "world")
      isEnumerable = !isEnumerable;
  }

  isConfigurable = delete emptyObject.prop1 && delete emptyObject.prop2

  assert(isWritable && isEnumerable && !isConfigurable);
})();

(function tc_15_02_03__016() {
  var emptyObject = {}

  var propertyDescriptor = {
    enumerable: true,
    configurable: true,
    get: function () {
      return myProperty;
    },
    set: function (newValue) {
      myProperty = newValue;
    }
  }

  Object.defineProperty(emptyObject, 'myProperty', propertyDescriptor);

  var checkGetSet = false;
  var isEnumerable = false;
  var isConfigurable = false;

  emptyObject.myProperty = "foobar";
  if (emptyObject.myProperty == "foobar")
    checkGetSet = true;

  for (p in emptyObject) {
    if (emptyObject[p] == "foobar") {
      isEnumerable = true;
      break;
    }
  }

  if (delete emptyObject.myProperty)
    isConfigurable = true;

  assert(checkGetSet && isEnumerable && isConfigurable);
})();

(function tc_15_02_03__010() {
  var a = new String("qwe");

  names = Object.getOwnPropertyNames(a);

  assert(names instanceof Array);

  var is_0 = false, is_1 = false, is_2 = false, is_length = false;
  for (var i = 0; i <= 3; i++)
  {
    if (names[i] === "0") { is_0 = true; }
    if (names[i] === "1") { is_1 = true; }
    if (names[i] === "2") { is_2 = true; }
    if (names[i] === "length") { is_length = true; }
  }

  assert (is_0 && is_1 && is_2 && is_length);
})();

(function tc_15_02_03__020() {
  var emptyObject = {}

  var propertyDescriptor = {
    enumerable: true,
    configurable: true,
    value: "hello!",
    writable: true
  }

  Object.defineProperty(emptyObject, 'myProperty', propertyDescriptor);

  var isWritable = false;
  var isEnumerable = false;
  var isConfigurable = false;

  emptyObject.myProperty = "foobar";
  if (emptyObject.myProperty == "foobar")
    isWritable = true;

  for (p in emptyObject) {
    if (emptyObject[p] == "foobar") {
      isEnumerable = true;
      break;
    }
  }

  if (delete emptyObject.myProperty)
    isConfigurable = true;

  assert(isWritable && isEnumerable && isConfigurable);
})();

(function tc_15_02_03__003() {
  writable = false;
  enumerable = false;
  configurable = false;

  Object.prototype = "qwerty";
  if (Object.prototype === "qwerty")
    writable = true;

  for (prop in Object)
  {
    if (Object[prop] == "qwerty")
      enumerable = true;
  }

  if (delete Object.prototype)
    configurable = true;

  assert(!writable && !enumerable && !configurable);
})();

(function tc_15_02_03__009() {
  var a = {
    prop1: Number,
    prop2: String,
    foo: function () {
      return 1;
    },
    bar: function () {
      return 0;
    }
  };
  names = Object.getOwnPropertyNames(a);

  assert(names instanceof Array &&
          names[0] === "prop1" &&
          names[1] === "prop2" &&
          names[2] === "foo" &&
          names[3] === "bar");
})();

(function tc_15_02_03__013() {
  var niceObject = {
    niceProp1: String,
    niceProp2: Number,
    niceMeth: function () {
      return true;
    }
  }
  var someElseObject = {
    prop1: Boolean,
    prop2: Number
  }
  var niceChild = Object.create(niceObject, someElseObject);

  assert(Object.getPrototypeOf(niceChild) === niceObject);
})();

(function tc_15_02_03__012() {
  var veryUsefulObject = {
  }

  Object.preventExtensions(veryUsefulObject);

  veryUsefulObject.property = "qwerty";

  veryUsefulObject.method = function () {
    return "asdf";
  }

  assert(veryUsefulObject.property === undefined);
  assert(veryUsefulObject.method === undefined);
})();

(function tc_15_02_03__008() {
  var object1 = {
    field1: 5,
    field2: "qwe",
    field3: true
  }

  var object2 = {
    field1: 5,
    field2: "qwe",
    field3: true
  }

  Object.freeze(object1);

  assert(Object.isFrozen(object1) && !Object.isFrozen(object2));
})();

(function tc_15_02_03__021() {
  var emptyObject = {}

  var propertyDescriptor = {
    enumerable: true,
    configurable: true,
    value: "hello!",
    writable: true
  }

  Object.defineProperty(emptyObject, 'myProperty', propertyDescriptor);

  var newPropertyDescriptor = {
    enumerable: false,
    configurable: false,
    writable: false
  }

  Object.defineProperty(emptyObject, 'myProperty', newPropertyDescriptor);

  var isWritable = false;
  var isEnumerable = false;
  var isConfigurable = false;

  emptyObject.myProperty = "foobar";
  if (emptyObject.myProperty == "foobar")
    isWritable = true;

  for (p in emptyObject) {
    if (emptyObject[p] == "foobar") {
      isEnumerable = true;
      break;
    }
  }

  if (delete emptyObject.myProperty)
    isConfigurable = true;

  assert(!isWritable && !isEnumerable && !isConfigurable);
})();

(function tc_15_02_03__007() {
  var a = {
    foo: function () {
      return 1
    }
  }
  desc = Object.getOwnPropertyDescriptor(a, "foo");

  assert(desc instanceof Object);
})();

(function tc_15_02_03__002() {
  assert(typeof Object.prototype == "object");
})();

(function tc_15_02_03__011() {
  var object1 = {
    field1: 5,
    field2: "qwe",
    field3: true
  }

  var object2 = {
    field1: 5,
    field2: "qwe",
    field3: true
  }

  Object.seal(object1);

  assert(Object.isSealed(object1) && !Object.isSealed(object2));
})();

(function tc_15_02_03__014() {
  var a = new String("qwe");
  var someElseObject = {
    prop1: Boolean,
    prop2: Number
  }
  var niceChild = Object.create(a, someElseObject);

  assert(Object.getPrototypeOf(niceChild) === a);
})();

(function tc_15_02_03__006() {
  var object1 = {
    field1: 5,
    field2: "qwe",
    field3: true
  }

  var object2 = {
    field1: 5,
    field2: "qwe",
    field3: true
  }

  Object.freeze(object1);

  assert(!Object.isExtensible(object1) && Object.isExtensible(object2));
})();

(function tc_15_02_03__015() {
  var a = {}
  var someElseObject = undefined;
  var childObject = Object.create(a, someElseObject);

  assert(Object.getPrototypeOf(childObject) === a && Object.getOwnPropertyNames(childObject).length == 0);
})();

(function tc_15_02_03__005() {
  var a = {
    field: Number
  }

  assert(Object.getPrototypeOf(a) == Object.getPrototypeOf(Object()));
})();

(function tc_15_02_04__002() {
  assert(Object.isExtensible(Object.getPrototypeOf(Object())));
})();

(function tc_15_02_04__001() {
  assert(Object.getPrototypeOf(Object.getPrototypeOf(Object())) === null);
})();

(function tc_15_02_04_07__001() {
  var object = {
    prop: true
  }

  assert(!object.propertyIsEnumerable('prop2'));
})();

(function tc_15_02_04_07__002() {
  var object = {}

  var propertyDescriptor = {
    enumerable: true,
    configurable: true,
    value: "qwe",
    writable: true
  }

  Object.defineProperty(object, 'prop', propertyDescriptor);

  assert(object.propertyIsEnumerable('prop'));
})();

(function tc_15_02_04_07__003() {
  var object = {}

  var propertyDescriptor = {
    enumerable: false,
    configurable: true,
    value: "qwe",
    writable: true
  }

  Object.defineProperty(object, 'prop', propertyDescriptor);

  assert(!object.propertyIsEnumerable('prop'));
})();

(function tc_15_02_04_04__007() {
  var a = false;

  assert(a.valueOf() === false);
})();

(function tc_15_02_04_04__004() {
  var a = new Number(5);

  assert(a.valueOf() === 5);
})();

(function tc_15_02_04_04__001() {
  var a = new Object();
  assert(a.valueOf() === a);
})();

(function tc_15_02_04_04__003() {
  var a = {
    n: true,
    s: "qwerty"
  }

  assert(a.valueOf() === a);
})();

(function tc_15_02_04_04__002() {
  var a = {
    n: Number(5)
  }
  assert(a.valueOf() === a);
})();

(function tc_15_02_04_04__008() {
  var a = new String("qwe");
  assert(a.valueOf() === "qwe");
})();

(function tc_15_02_04_04__009() {
  var a = "asdfgh";
  assert(a.valueOf() === "asdfgh");
})();

(function tc_15_02_04_04__005() {
  var a = 123;

  assert(a.valueOf() === 123);
})();

(function tc_15_02_04_04__010() {
  var a = "123";
  assert(a.valueOf() === "123");
})();

(function tc_15_02_04_04__006() {
  var a = new Boolean(true);

  assert(a.valueOf() === true);
})();

(function tc_15_02_04_02__003() {
  var obj = {};
  assert(obj.toString() === "[object Object]");
})();

(function tc_15_02_04_02__001() {
  assert(Object.prototype.toString.call(undefined) === "[object Undefined]");
})();

(function tc_15_02_04_02__002() {
  assert(Object.prototype.toString.call(null) === "[object Null]");
})();

(function tc_15_02_04_02__004() {
  assert(Object.prototype.toString.call(123) === "[object Number]");
})();

(function tc_15_02_04_05__002() {
  var obj = {
    prop1: 5
  }

  assert(!obj.hasOwnProperty("prop5"));
})();

(function tc_15_02_04_05__004() {
  function Parent() {
    this.prop = 5;
  }

  function Child() {
    this.prop2 = false;
  }

  Child.prototype = Parent;

  var obj = new Child();

  assert(obj.hasOwnProperty("prop2"));
})();

(function tc_15_02_04_05__003() {
  function Parent() {
    this.prop = 5;
  }

  function Child() {
    this.prop2 = false;
  }

  Child.prototype = Parent;

  var obj = new Child();

  assert(!obj.hasOwnProperty("prop"));
})();

(function tc_15_02_04_05__001() {
  var obj = {
    prop1: 5,
    prop2: "qwe",
    prop3: Boolean
  }

  assert(obj.hasOwnProperty("prop1"));
})();

(function tc_15_02_04_01__002() {
  assert(Object.prototype.constructor === Object);
})();

(function tc_15_02_04_01__001() {
  assert(Object.getPrototypeOf(Object()).constructor === Object);
})();

(function tc_15_02_04_03__003() {
  assert(Object.toLocaleString() === Object.toString());
})();

(function tc_15_02_04_03__002() {
  var o = new Object();
  assert(o.toLocaleString() === o.toString());

})();

(function tc_15_02_04_03__001() {
  assert(typeof Object.prototype.toLocaleString === 'function');
})();

(function tc_15_02_04_06__001() {
  var a = new Object();
  var b = 123;

  assert(!a.isPrototypeOf(b));
})();

(function tc_15_02_04_06__002() {
  var object = new Object();
  var p = Object.getPrototypeOf(object);

  assert(p.isPrototypeOf(object));
})();

(function tc_15_02_04_06__006() {
  try
  {
    Object.prototype.isPrototypeOf.call(undefined, {});

    assert(false);
  } catch (e)
  {
    assert (e instanceof TypeError);
  }
})();

(function tc_15_02_04_06__003() {
  var object = new Object();
  var otherObject = new Object();

  assert(!otherObject.isPrototypeOf(object));
})();

(function tc_15_02_04_06__005() {
  var object = Object.create(null);
  var temp = new Object();
  assert(!temp.isPrototypeOf(object));
})();

(function tc_15_02_04_06__004() {
  var object = new Object();
  assert(!object.isPrototypeOf(object));
})();

(function tc_15_02_02__009() {
  var a = new Object(null);
  assert(typeof a === 'object' &&
          typeof (Object.getPrototypeOf(a)) === "object" &&
          Object.isExtensible(a));
})();

(function tc_15_02_02__002() {
  var a = new Object();
  var b = new Object();

  assert(a !== b);
})();

(function tc_15_02_02__006() {
  var a = 5.5;
  var b = new Object(a);
  assert(typeof b === "object" && b == a && b !== a);
})();

(function tc_15_02_02__004() {
  var a = {
    field1: Number,
    field2: String,
    foo: function () {
      return 0;
    }
  }
  var b = new Object(a);
  assert(a === b);
})();

(function tc_15_02_02__008() {
  var a = new Object();
  assert(typeof a === 'object' &&
          typeof (Object.getPrototypeOf(a)) === "object" &&
          Object.isExtensible(a));

})();

(function tc_15_02_02__010() {
  var a = new Object(undefined);
  assert(typeof a === 'object' &&
          typeof (Object.getPrototypeOf(a)) === "object" &&
          Object.isExtensible(a));

})();

(function tc_15_02_02__007() {
  var a = true;
  var b = new Object(a);
  assert(typeof b === "object" && b == a && b !== a);
})();

(function tc_15_02_02__005() {
  var a = "foobar";
  var b = new Object(a);
  assert(typeof b === "object" && b == a && b !== a);
})();

(function tc_15_02_02__003() {
  var a = new Object();
  var b = new Object(a);
  assert(a === b);
})();

(function tc_15_02_02__001() {
  var a = new Object();

  assert(typeof a === 'object');
})();

(function tc_15_03_02_01__002() {
  try
  {
    Function('a', 'a', '"use strict";');
    assert(false);
  }
  catch (e)
  {
  }
})();

(function tc_15_03_02_01__005() {
  "use strict";
  try
  {
    Function('eval', 'return;');

  }
  catch (e)
  {
    assert(false);
  }
})();

(function tc_15_03_02_01__009() {
  "use strict";

  try
  {
    Function('arguments', 'return;');
  }
  catch (e)
  {
    assert(false);
  }
})();

(function tc_15_03_02_01__004() {
  try
  {
    Function('eval', '"use strict";');
    assert(false);
  }
  catch (e)
  {
  }
})();

(function tc_15_03_02_01__011() {
  "use strict";

  var foo = new Function("baz", "baz", "baz", "return 0;");

  assert(foo() === 0);
})();

(function tc_15_03_02_01__001() {
  var func = new Function("a,b", "c", "return a+b+c")
  assert(func(1, 2, 3) == 6);
})();

(function tc_15_03_02_01__010() {
  "use strict";

  var foo = new Function("baz", "qux", "baz", "return 0;");
})();

(function tc_15_03_02_01__008() {
  "use strict";

  try
  {
    Function('a,a', 'return a;');
  }
  catch (e)
  {
    assert(false);
  }
})();

(function tc_15_03_02_01__007() {
  try
  {
    Function('a,a', '"use strict";');
    assert(false);
  }
  catch (e)
  {
  }
})();

(function tc_15_03_02_01__012s() {
  "use strict";

  try
  {
    Function('a', 'a', 'return;');
  }
  catch (e)
  {
    assert(false);
  }
})();

(function tc_15_03_04_02__005() {
  assert(Function.prototype.toString.hasOwnProperty('length'));

  var obj = Function.prototype.toString.length;

  Function.prototype.toString.length = function () {
    return "shifted";
  };

  assert(Function.prototype.toString.length === obj);
})();

(function tc_15_03_04_02__002() {
  var FACTORY = Function.prototype.toString;

  try
  {
    var instance = new FACTORY;
    assert(false);
  }
  catch (e)
  {
  }
})();

(function tc_15_03_04_02__003() {
  assert(Function.prototype.toString.hasOwnProperty('length'));
  assert(!Function.prototype.toString.propertyIsEnumerable('length'));
  for (p in Function.prototype.toString)
  {
    assert(p !== "length");
  }
})();

(function tc_15_03_04_02__006() {
  assert(Function.prototype.toString.hasOwnProperty("length"));
  assert(Function.prototype.toString.length === 0);

})();

(function tc_15_03_03__004() {
  assert(Function.hasOwnProperty("length"));
  assert(Function.length === 1);
})();

(function tc_15_03_03__002() {
  assert(Function.prototype.isPrototypeOf(Function));
})();

(function tc_15_03_03__003() {
  Function.prototype.indicator = 1;
  assert (Function.indicator === 1);
})();

(function tc_15_03_03__001() {
  assert(Function.hasOwnProperty("prototype"));
})();

(function tc_15_03_03_01__001() {
  var obj = Function.prototype;
  Function.prototype = function () {
    return "shifted";
  };

  if (Function.prototype !== obj)
  {
    assert(false);
  }

  try
  {
    if (Function.prototype() !== undefined)
    {
      assert(false);
    }
  }
  catch (e)
  {
    assert(false);
  }
})();

(function tc_15_03_03_01__002() {
  if (Function.propertyIsEnumerable('prototype'))
  {
    assert(false);
  }

  var count = 0;

  for (p in Function)
  {
    if (p === "prototype")
      count++;
  }

  if (count !== 0)
  {
    assert(false);
  }
})();

(function tc_15_03_03_01__003() {
  delete Function.prototype;

  if (!(Function.hasOwnProperty('prototype')))
  {
    assert(false);
  }
})();

(function tc_15_03_03_01__004() {
  function foo() {
  }

  Object.defineProperty(foo, 'prototype', {value: {}});
  assert(foo.prototype ===
          Object.getOwnPropertyDescriptor(foo, 'prototype').value);

})();

(function tc_15_07__001() {
  var a = Number;
  Number = null;
  var b = new a(5);
  Number = a;
  assert(b !== 5);
})();

(function tc_15_07__002() {
  var a = Number;
  Number = null;
  var b = new a(5)
  Number = a;
  assert(!(b === 5));
})();

(function tc_15_07_01__010() {
  var a = Number;
  Number = null;
  var b = a(2);
  Number = a;
  assert(b === 2 && typeof b === "number");
})();

(function tc_15_07_01__002() {
  assert (typeof Number("123456") === "number");
})();

(function tc_15_07_01__005() {
  assert (Number() === +0);
})();

(function tc_15_07_01__006() {
  assert(isNaN(Number(new Error())));
})();

(function tc_15_07_01__008() {
  assert(isNaN(Number("abcdefg")));
})();

(function tc_15_07_01__004() {
  assert (Number(753) === 753);
})();

(function tc_15_07_01__001() {
  assert(Number("123456") === 123456);
})();

(function tc_15_07_01__007() {
  assert(typeof Number("abcdefg") === "number");
})();

(function tc_15_07_01__003() {
  assert(typeof Number(new Object()) === "number");
})();

(function tc_15_07_01__009() {
  assert(isNaN(Number(function a() {return Infinity})));
})();

(function tc_15_07_02__011() {
  var a = new Number();
  assert(a.valueOf() === +0.0);
})();

(function tc_15_07_02__004() {
  var a = Number
  Number = null
  var b = new a("1e12")
  Number = a;
  assert(b.toString(35) === "fiyo05lf");
})();

(function tc_15_07_02__007() {
  ts = Number.prototype.toString
  delete Number.prototype.toString;
  var a = new Number()
  assert(a.toString() === "[object Number]");
  Number.prototype.toString = ts;
})();

(function tc_15_07_02__001() {
  var a = new Number("123456");
  assert((a == 123456) && (typeof a === 'object'));
})();

(function tc_15_07_02__006() {
  var a = Number
  Number = null
  var b = new a("1e12")
  b.c = new a(new a(777))
  Number = a;
  assert(typeof b.c === "object" && b.c.valueOf() === 777)
})();

(function tc_15_07_02__003() {
  var a = Number
  Number = null
  var b = new a("1e12")
  Number = a;
  assert(b == 1000000000000 && typeof b === "object");
})();

(function tc_15_07_02__005() {
  var a = Number
  Number = null
  var b = new a("1e12")
  b.c = new a(new Error())
  Number = a;

  assert(typeof b.c === "object" && isNaN(b.c));
})();

(function tc_15_07_02__010() {
  var b = Number.prototype
  var a = Number
  Number = null
  var c = new a(5)
  Number = a;
  assert(b === c.constructor.prototype);
})();

(function tc_15_07_02__009() {
  var a = new Number(null)
  assert(Number.prototype === a.constructor.prototype);
})();

(function tc_15_07_02__002() {
  var a = new Number();
  assert((a == +0.0) && (typeof a === 'object'));
})();

(function tc_15_07_02__008() {
  var a = new Number(null)
  assert(Number.prototype.isPrototypeOf(a));
})();

(function tc_15_07_04__003() {
  assert(Object.prototype.isPrototypeOf(Number.prototype));
})();

(function tc_15_07_04_01__002() {
  assert(Number.prototype.constructor === Number);
})();

(function tc_15_07_04_01__001() {
  assert(Number.prototype.hasOwnProperty('constructor'));
})();

(function tc_15_07_04_02__012() {
  assert((new Number(Number.POSITIVE_INFINITY)).toString(undefined) === "Infinity");
})();

(function tc_15_07_04_02__011() {
  assert((new Number(NaN)).toString(undefined) === "NaN")
})();

(function tc_15_07_04_02__005() {
  var a = -123456789012345
  assert(a.toString(8) === "-3404420603357571");
})();

(function tc_15_07_04_02__003() {
  var a = new Number(15);
  assert(a.toString(2) === "1111");
})();

(function tc_15_07_04_02__010() {
  assert((new Number(NaN)).toString() === "NaN");
})();

(function tc_15_07_04_02__001() {
  var a = Number(0.1);
  assert(a.toString(36) === "0.3lllllllllm");
})();

(function tc_15_07_04_02__013() {
  assert ((new Number(Number.NEGATIVE_INFINITY)).toString(undefined) === "-Infinity");
})();

(function tc_15_07_04_02__004() {
  var a = 123456789012345
  assert(a.toString(8) === "3404420603357571");
})();

(function tc_15_07_04_02__009() {
  assert(Number.prototype.hasOwnProperty('toString') &&
          typeof Number.prototype.toString === "function");
})();

(function tc_15_07_03__004() {
  var p = Object.getPrototypeOf(Number);
  assert(p === Function.prototype);
})();

(function tc_15_07_03__002() {
  assert(Number.hasOwnProperty("length") && Number.length === 1);
})();

(function tc_15_07_03__003() {
  assert(Function.prototype.isPrototypeOf(Number) === true);
})();

(function tc_15_07_03__001() {
  assert(Number.constructor.prototype === Function.prototype);
})();

(function tc_15_07_03_02__002() {
  assert(Number.MAX_VALUE === 1.7976931348623157e308);
})();

(function tc_15_07_03_02__003() {
  assert(Number.MAX_VALUE === 1.7976931348623157e308);
})();

(function tc_15_07_03_02__004() {
  var b = Number.MAX_VALUE;
  Number.MAX_VALUE = 0;
  assert(Number.MAX_VALUE === b);
})();

(function tc_15_07_03_02__006() {
  for (x in Number)
  {
    if (x === "MAX_VALUE")
    {
      assert(false);
    }
  }
})();

(function tc_15_07_03_02__005() {
  assert(!(delete Number.MAX_VALUE));
})();

(function tc_15_07_03_02__001() {
  assert(Number.hasOwnProperty("MAX_VALUE"));
})();

(function tc_15_07_03_01__001() {
  assert(Number.hasOwnProperty("prototype"));
})();

(function tc_15_07_03_01__002() {
  var a = Object.getOwnPropertyDescriptor(Number, 'prototype');

  assert((a.writable === false &&
          a.enumerable === false &&
          a.configurable === false));
})();

(function tc_15_07_03_01__003() {
  assert(Object.getPrototypeOf(new Number()) === Number.prototype);
})();

(function tc_15_07_03_01__007() {
  for (x in Number)
  {
    if (x === "prototype")
    {
      assert(false);
    }
  }
})();

(function tc_15_07_03_01__005() {
  assert(delete Number.prototype === false)
})();

(function tc_15_07_03_01__006() {
  assert(!Number.propertyIsEnumerable('prototype'));
})();

(function tc_15_07_03_06__001() {
  assert(Number.hasOwnProperty("POSITIVE_INFINITY"));
})();

(function tc_15_07_03_06__006() {
  for (x in Number)
  {
    if (x === "POSITIVE_INFINITY")
    {
      assert(false);
    }
  }
})();

(function tc_15_07_03_06__007() {
  assert(!Number.propertyIsEnumerable('POSITIVE_INFINITY'));
})();

(function tc_15_07_03_06__003() {
  assert(Number.POSITIVE_INFINITY === Infinity);
})();

(function tc_15_07_03_06__005() {
  assert(!(delete Number.POSITIVE_INFINITY));
})();

(function tc_15_07_03_06__002() {
  assert(!isFinite(Number.POSITIVE_INFINITY) && Number.POSITIVE_INFINITY > 0);
})();

(function tc_15_07_03_06__004() {
  var b = Number.POSITIVE_INFINITY
  Number.POSITIVE_INFINITY = 0
  assert(Number.POSITIVE_INFINITY === b);
})();

(function tc_15_07_03_05__005() {
  assert(!(delete Number.NEGATIVE_INFINITY));
})();

(function tc_15_07_03_05__006() {
  for (x in Number)
  {
    if (x === "NEGATIVE_INFINITY")
    {
      assert(false);
    }
  }
})();

(function tc_15_07_03_05__007() {
  assert(!Number.propertyIsEnumerable('NEGATIVE_INFINITY'));
})();

(function tc_15_07_03_05__003() {
  assert(Number.NEGATIVE_INFINITY === -Infinity);
})();

(function tc_15_07_03_05__001() {
  assert(Number.hasOwnProperty("NEGATIVE_INFINITY"));
})();

(function tc_15_07_03_05__002() {
  assert(!(isFinite(Number.NEGATIVE_INFINITY) && Number.NEGATIVE_INFINITY < 0));
})();

(function tc_15_07_03_05__004() {
  var b = Number.NEGATIVE_INFINITY;
  Number.NEGATIVE_INFINITY = 0;
  assert(Number.NEGATIVE_INFINITY === b);
})();

(function tc_15_07_03_04__001() {
  assert(Number.hasOwnProperty("NaN"));
})();

(function tc_15_07_03_04__005() {
  assert(!(delete Number.NaN));
})();

(function tc_15_07_03_04__003() {
  for (x in Number)
  {
    if (x === "NaN")
    {
      assert(false);
    }
  }
})();

(function tc_15_07_03_04__004() {
  Number.NaN = 0;
  assert(isNaN(Number.NaN));
})();

(function tc_15_07_03_04__002() {
  assert(isNaN(Number.NaN));
})();

(function tc_15_07_03_03__001() {
  assert(Number.hasOwnProperty("MIN_VALUE"));
})();

(function tc_15_07_03_03__005() {
  assert(!(delete Number.MIN_VALUE));
})();

(function tc_15_07_03_03__003() {
  assert(Number.MIN_VALUE === 5e-324);
})();

(function tc_15_07_03_03__006() {
  for (x in Number)
  {
    if (x === "MIN_VALUE")
    {
      assert(false);
    }
  }
})();

(function tc_15_07_03_03__004() {
  var b = Number.MIN_VALUE
  Number.MIN_VALUE = 0
  assert(Number.MIN_VALUE === b);
})();

(function tc_15_07_03_03__002() {
  assert(Number.MIN_VALUE === 5e-324);
})();

(function tc_15_05_02_01__002() {
  var s = new String ("");
  s.x = 1;
  assert (s.x === 1);
})();

(function tc_15_05_02_01__001() {
  assert(String.prototype.isPrototypeOf(new String("")));
})();

(function tc_15_05_03_01__002() {
  for (var p in String) {
    if (p === String.prototype) {
      assert(false);
    }
  }
})();

(function tc_15_05_03_01__001() {
  String.prototype = 1;
  assert(String.prototype !== 1);
})();

(function tc_15_05_03_02__001() {
  assert (String.fromCharCode () === "");
})();

(function tc_15_05_03_02__002() {
  assert (String.fromCharCode (65, 66, 67) === "ABC");
})();

(function tc_15_05_01_01__005() {
  assert (String (false) === "false");
})();

(function tc_15_05_01_01__008() {
  assert (String (-0) === "0");
})();

(function tc_15_05_01_01__013() {
  assert (String (0.111111111111111) === "0.111111111111111");
})();

(function tc_15_05_01_01__010() {
  assert (String (Infinity) === "Infinity");
})();

(function tc_15_05_01_01__007() {
  assert (String (+0) === "0");
})();

(function tc_15_05_01_01__009() {
  assert (String (-1) === "-" + String (1));
})();

(function tc_15_05_01_01__012() {
  assert (String (10000000000000000000) === "10000000000000000000");
})();

(function tc_15_05_01_01__006() {
  assert(String(NaN) === "NaN");
})();

(function tc_15_05_01_01__001() {
  assert (String () === String (""));
})();

(function tc_15_05_01_01__015() {
  assert (String (0.000000111111111111111) === "1.11111111111111e-7");
})();

(function tc_15_05_01_01__004() {
  assert (String (true) === "true");
})();

(function tc_15_05_01_01__014() {
  assert (String (0.00000111111111111111) === "0.00000111111111111111");
})();

(function tc_15_05_01_01__002() {
  assert (String (undefined) === "undefined");
})();

(function tc_15_05_01_01__003() {
  assert (String (null) === "null");
})();

(function tc_15_05_01_01__011() {
  assert (String (123000) === "123000");
})();

(function tc_15_05_04_07__001() {
  assert(String("abcd").indexOf("ab") === 0);
})();

(function tc_15_05_04_07__002() {
  assert(String("abcd").indexOf("ab", 0) === 0);
})();

(function tc_15_05_04_07__003() {
  assert(String("abcd").indexOf("ab", 1) === -1);
})();

(function tc_15_05_04_01__001() {
  assert (String.prototype.constructor === String);
})();

(function tc_15_05_04_05__001() {
  assert(isNaN(String("abc").charCodeAt(-1)));
})();

(function tc_15_05_04_05__002() {
  assert(isNaN(String("abc").charCodeAt(3)));
})();

(function tc_15_05_04_05__004() {
  assert(String("abc").charCodeAt("0") === 97);
})();

(function tc_15_05_04_05__003() {
  assert(String("abc").charCodeAt(0) === 97);
})();

(function tc_15_05_04_03__001() {
  assert (String ("abc").valueOf () === "abc");
})();

(function tc_15_05_04_02__001() {
  assert (String ("abc").toString () === "abc");
})();

(function tc_15_05_04_02__002() {
  assert ("abc".toString () === "abc");
})();

(function tc_15_05_04_04__003() {
  assert(String("abc").charAt(0) === "a");
})();

(function tc_15_05_04_04__004() {
  assert(String("abc").charAt("0") === "a");
})();

(function tc_15_05_04_04__001() {
  assert(String("abc").charAt(-1) === "");
})();

(function tc_15_05_04_04__002() {
  assert(String("abc").charAt(3) === "");
})();

(function tc_15_05_04_06__004() {
  assert (String ().concat.length === 1);
})();

(function tc_15_05_04_06__001() {
  assert(String("abc").concat("d") === "abcd");
})();

(function tc_15_05_04_06__003() {
  assert(String().concat("a", "b", "c") === "abc");
})();

(function tc_15_05_04_06__002() {
  assert(String().concat() === "");
})();

(function tc_15_08_02_06__011() {
  assert(isNaN(Math.ceil("NaN")));
})();

(function tc_15_08_02_06__007() {
  assert (Math.ceil(-1.3) === -1);
})();

(function tc_15_08_02_06__006() {
  assert(1/Math.ceil(-0.3) === -Infinity);
})();

(function tc_15_08_02_06__012() {
  assert(isNaN(Math.ceil(new Object())));
})();

(function tc_15_08_02_06__003() {
  assert(1/Math.ceil(-0) === -Infinity);
})();

(function tc_15_08_02_06__010() {
  assert (Math.ceil(1.1) === 2);
})();

(function tc_15_08_02_06__004() {
  assert(Math.ceil(-Infinity) === -Infinity);
})();

(function tc_15_08_02_06__009() {
  assert (Math.ceil(1.9) === 2);
})();

(function tc_15_08_02_06__008() {
  assert (Math.ceil(-1.9) === -1);
})();

(function tc_15_08_02_06__005() {
  assert(Math.ceil(Infinity) === Number.POSITIVE_INFINITY);
})();

(function tc_15_08_02_06__002() {
  var res = 1 / Math.ceil(+0)
  assert(res === +Infinity && res !== -Infinity);
})();

(function tc_15_08_02_06__001() {
  assert(isNaN(Math.ceil(NaN)));
})();

(function tc_15_08_02_16__005() {
  assert (isNaN(Math.sin(-Infinity)));
})();

(function tc_15_08_02_16__001() {
  assert (isNaN(Math.sin(NaN)));
})();

(function tc_15_08_02_16__003() {
  assert (1/Math.sin(-0) === -Infinity);
})();

(function tc_15_08_02_16__004() {
  assert (isNaN(Math.sin(Infinity)));
})();

(function tc_15_08_02_16__002() {
  assert (1/Math.sin(+0) === Infinity);
})();

(function tc_15_08_02_03__009() {
  assert(Math.asin(1) === Math.PI / 2);
})();

(function tc_15_08_02_03__007() {
  assert(Math.asin(+0) === +0);
})();

(function tc_15_08_02_03__004() {
  assert(!isNaN(Math.asin(-1.0000000000000001)));
})();

(function tc_15_08_02_03__005() {
  assert(isNaN(Math.asin(-1.000000000000001)));
})();

(function tc_15_08_02_03__003() {
  assert(!isNaN(Math.asin(1.0000000000000001)));
})();

(function tc_15_08_02_03__008() {
  assert(Math.asin(-0) === -0);
})();

(function tc_15_08_02_03__006() {
  assert(isNaN(Math.asin(-3)));
})();

(function tc_15_08_02_03__001() {
  assert(isNaN(Math.asin(NaN)));
})();

(function tc_15_08_02_03__002() {
  assert(isNaN(Math.asin(1.000000000000001)));
})();

(function tc_15_08_02_17__003() {
  assert (1/Math.sqrt(+0) === Infinity);
})();

(function tc_15_08_02_17__005() {
  assert (Math.sqrt(Infinity) === Infinity);
})();

(function tc_15_08_02_17__004() {
  assert (1/Math.sqrt(-0) === -Infinity);
})();

(function tc_15_08_02_17__002() {
  assert (isNaN(Math.sqrt(-2)));
})();

(function tc_15_08_02_17__001() {
  assert (isNaN(Math.sqrt(NaN)));
})();

(function tc_15_08_02_01__002() {
  assert(Math.abs(-0.0) === +0.0);
})();

(function tc_15_08_02_01__001() {
  assert(isNaN(Math.abs(NaN)));
})();

(function tc_15_08_02_01__005() {
  assert(Math.abs(-123513745) === 123513745);
})();

(function tc_15_08_02_01__003() {
  assert(Math.abs(Number.NEGATIVE_INFINITY) === Number.POSITIVE_INFINITY);
})();

(function tc_15_08_02_01__004() {
  assert(Math.abs(Number.NEGATIVE_INFINITY) === Number.POSITIVE_INFINITY);
})();

(function tc_15_08_02_11__004() {
  assert(isNaN(Math.max(NaN)));
})();

(function tc_15_08_02_11__012() {
  assert(typeof Math.max === "function");
})();

(function tc_15_08_02_11__009() {
  assert(Math.max() === -Infinity);
})();

(function tc_15_08_02_11__003() {
  assert(isNaN(Math.max(Object())));
})();

(function tc_15_08_02_11__011() {
  assert(Math.max(+0, -0) === +0);
})();

(function tc_15_08_02_11__006() {
  assert(isNaN(Math.max(5, -7, NaN)));
})();

(function tc_15_08_02_11__001() {
  assert(isNaN(Math.max(undefined)));
})();

(function tc_15_08_02_11__002() {
  assert(isNaN(Math.max({})));
})();

(function tc_15_08_02_11__010() {
  assert(Math.max() !== Infinity);
})();

(function tc_15_08_02_11__014() {
  assert(1/Math.max(-0, +0) === Infinity);
})();

(function tc_15_08_02_11__005() {
  assert(isNaN(Math.max(5, 7, NaN)));
})();

(function tc_15_08_02_11__008() {
  assert(!isFinite(Math.max()));
})();

(function tc_15_08_02_11__007() {
  assert(Math.max(5, -7) === 5);
})();

(function tc_15_08_02_11__013() {
  assert(Math.max.length === 2);
})();

(function tc_15_08_02_07__003() {
  assert (Math.cos(+0) === 1);
})();

(function tc_15_08_02_07__006() {
  assert (isNaN(Math.cos(-Infinity)));
})();

(function tc_15_08_02_07__001() {
  assert (isNaN(Math.cos(NaN)));
})();

(function tc_15_08_02_07__004() {
  assert (Math.cos(-0) === 1);
})();

(function tc_15_08_02_07__007() {
  assert (Math.cos(Math.PI) === -1);
})();

(function tc_15_08_02_07__002() {
  assert(isNaN(Math.cos("   NaN")));
})();

(function tc_15_08_02_07__005() {
  assert (isNaN(Math.cos(Infinity)));
})();

(function tc_15_08_02_10__002() {
  assert (isNaN(Math.log(-0.00001)));
})();

(function tc_15_08_02_10__004() {
  assert (Math.log(-0) === -Infinity);
})();

(function tc_15_08_02_10__001() {
  assert(isNaN(Math.log(NaN)));
})();

(function tc_15_08_02_10__005() {
  assert (1/Math.log(1) === Infinity);
})();

(function tc_15_08_02_10__003() {
  assert(Math.log(+0) === -Infinity);
})();

(function tc_15_08_02_10__006() {
  assert (Math.log(Infinity) === Infinity);
})();

(function tc_15_08_02_13__029() {
  assert (Math.pow(2,2) === 4);
})();

(function tc_15_08_02_13__022() {
  assert (1/Math.pow(+0, 5.2) === Infinity);
})();

(function tc_15_08_02_13__010() {
  assert(isNaN(Math.pow(1, Infinity)));
})();

(function tc_15_08_02_13__023() {
  assert(Math.pow(+0, -5.2) === Infinity);

})();

(function tc_15_08_02_13__017() {
  assert (1/Math.pow(Infinity, -3) === Infinity);
})();

(function tc_15_08_02_13__024() {
  assert (1/Math.pow(-0, 12) === Infinity);
})();

(function tc_15_08_02_13__031() {
  assert(isNaN(Math.pow(1, NaN)));
})();

(function tc_15_08_02_13__007() {
  assert (1/Math.pow(5, -Infinity) === Infinity);
})();

(function tc_15_08_02_13__027() {
  assert (Math.pow(-0, -1) === -Infinity);
})();

(function tc_15_08_02_13__014() {
  assert(1 / Math.pow(0.3, Infinity) === Infinity);
})();

(function tc_15_08_02_13__003() {
  assert (Math.pow(NaN, +0, 5, "qeqegfhb") === 1);
})();

(function tc_15_08_02_13__002() {
  assert(Math.pow(2, +0, 5, "qeqegfhb") === 1);
})();

(function tc_15_08_02_13__009() {
  assert(Math.pow(-5, Infinity) === Infinity);
})();

(function tc_15_08_02_13__021() {
  assert (1/Math.pow(-Infinity, -5) === -Infinity);
})();

(function tc_15_08_02_13__001() {
  assert(isNaN(Math.pow(2, "NaN", 5)));
})();

(function tc_15_08_02_13__025() {
  assert (1/Math.pow(-0, 7) === -Infinity);
})();

(function tc_15_08_02_13__012() {
  assert (isNaN(Math.pow(-1, -Infinity)));
})();

(function tc_15_08_02_13__016() {
  assert (Math.pow(Infinity, 3) === Infinity);
})();

(function tc_15_08_02_13__013() {
  assert(isNaN(Math.pow(1, -Infinity)));
})();

(function tc_15_08_02_13__011() {
  assert (isNaN(Math.pow(-1, Infinity)));
})();

(function tc_15_08_02_13__015() {
  assert (Math.pow(-0.3, -Infinity) === Infinity);
})();

(function tc_15_08_02_13__028() {
  assert(isNaN(Math.pow(-174, 1.78)));
})();

(function tc_15_08_02_13__008() {
  assert (1/Math.pow(-5, -Infinity) === Infinity);
})();

(function tc_15_08_02_13__006() {
  assert (Math.pow(5, Infinity) === Infinity);
})();

(function tc_15_08_02_13__026() {
  assert (Math.pow(-0, -100) === Infinity);
})();

(function tc_15_08_02_13__004() {
  assert (Math.pow("qeqegfhb", -0) === 1);
})();

(function tc_15_08_02_13__030() {
  assert (Math.pow("2   ","2.0") === 4);
})();

(function tc_15_08_02_13__005() {
  assert (isNaN(Math.pow("qeqegfhb", 1)));
})();

(function tc_15_08_02_13__020() {
  assert (1/Math.pow(-Infinity, -6) === Infinity);
})();

(function tc_15_08_02_13__019() {
  assert (Math.pow(-Infinity, 5) === -Infinity);
})();

(function tc_15_08_02_13__018() {
  assert (Math.pow(-Infinity, 6) === Infinity);
})();

(function tc_15_08_02_15__006() {
  assert (1/Math.round(0.2) === Infinity);
})();

(function tc_15_08_02_15__003() {
  assert (1/Math.round(-0) === -Infinity);
})();

(function tc_15_08_02_15__007() {
  assert (1/Math.round(-0.3) === -Infinity);
})();

(function tc_15_08_02_15__005() {
  assert (Math.round(-Infinity) === -Infinity);
})();

(function tc_15_08_02_15__001() {
  assert (isNaN(Math.round(NaN)));
})();

(function tc_15_08_02_15__004() {
  assert (Math.round(Infinity) === Infinity);
})();

(function tc_15_08_02_15__002() {
  assert (1/Math.round(+0) === Infinity);
})();

(function tc_15_08_02_05__003() {
  assert(Math.atan2(Number.MIN_VALUE, +0) === Math.PI / 2);
})();

(function tc_15_08_02_05__020() {
  assert(Math.atan2(-Number.MAX_VALUE, Number.POSITIVE_INFINITY) === -0);
})();

(function tc_15_08_02_05__019() {
  assert(Math.atan2(Number.MAX_VALUE, -Infinity) === Math.PI);
})();

(function tc_15_08_02_05__008() {
  assert(Math.atan2(+0, -0) === Math.PI);
})();

(function tc_15_08_02_05__029() {
  assert(Math.atan2(-Infinity, -Infinity) === -3*Math.PI / 4);
})();

(function tc_15_08_02_05__016() {
  assert(Math.atan2(-99999999, +0) === -Math.PI/2);
})();

(function tc_15_08_02_05__028() {
  assert(Math.atan2(-Infinity, +Infinity) === -Math.PI / 4);
})();

(function tc_15_08_02_05__007() {
  assert(Math.atan2(+0, +0) === +0);
})();

(function tc_15_08_02_05__006() {
  assert(Math.atan2(+0, Number.MAX_VALUE) === +0);
})();

(function tc_15_08_02_05__009() {
  assert(Math.atan2(+0, -Number.MIN_VALUE) === Math.PI);
})();

(function tc_15_08_02_05__025() {
  assert(Math.atan2(-Infinity, -999999999) === -Math.PI / 2);
})();

(function tc_15_08_02_05__014() {
  assert(Math.atan2(-0, -Number.MIN_VALUE) === -Math.PI);
})();

(function tc_15_08_02_05__026() {
  assert(Math.atan2(Infinity, Infinity) === Math.PI / 4);
})();

(function tc_15_08_02_05__023() {
  assert(Math.atan2(Infinity, -1) === Math.PI / 2);
})();

(function tc_15_08_02_05__024() {
  assert(Math.atan2(-Infinity, -1) === -Math.PI / 2);
})();

(function tc_15_08_02_05__011() {
  assert(Math.atan2(-0, Infinity) === -0);
})();

(function tc_15_08_02_05__010() {
  assert(Math.atan2(+0, -Infinity) === Math.PI);
})();

(function tc_15_08_02_05__015() {
  assert(Math.atan2(-0, -Infinity) === -Math.PI);
})();

(function tc_15_08_02_05__004() {
  assert(!(Math.atan2(0, +0) === Math.PI / 2));
})();

(function tc_15_08_02_05__021() {
  assert(Math.atan2(-Number.MIN_VALUE, Number.NEGATIVE_INFINITY) === -Math.PI);
})();

(function tc_15_08_02_05__022() {
  assert(Math.atan2(Infinity, 1) === Math.PI / 2);
})();

(function tc_15_08_02_05__013() {
  assert(Math.atan2(-0, -0) === -Math.PI);
})();

(function tc_15_08_02_05__027() {
  assert(Math.atan2(Infinity, -Infinity) === 3*Math.PI / 4);
})();

(function tc_15_08_02_05__001() {
  assert(isNaN(Math.atan2(NaN, 1)));
})();

(function tc_15_08_02_05__002() {
  assert(isNaN(Math.atan2(1, NaN)));
})();

(function tc_15_08_02_05__017() {
  assert(Math.atan2(-99999999, -0) === -Math.PI/2);
})();

(function tc_15_08_02_05__005() {
  assert(Math.atan2(1, -0) === Math.PI / 2);
})();

(function tc_15_08_02_05__018() {
  assert(Math.atan2(1, Infinity) === +0);
})();

(function tc_15_08_02_05__012() {
  assert(Math.atan2(-0, +0) === -0);
})();

(function tc_15_08_02_02__002() {
  assert(!isNaN(Math.acos(1.00000000000000000000001)));
})();

(function tc_15_08_02_02__003() {
  assert(isNaN(Math.acos(Number.NEGATIVE_INFINITY)));
})();

(function tc_15_08_02_02__005() {
  assert(isNaN(Math.acos(1.000000000000001)));
})();

(function tc_15_08_02_02__004() {
  assert(Math.acos(1) === +0.0);
})();

(function tc_15_08_02_02__006() {
  assert(isNaN(Math.acos(-7)));
})();

(function tc_15_08_02_02__001() {
  assert(isNaN(Math.acos(NaN)));
})();

(function tc_15_08_02_12__010() {
  assert(Math.min() !== -Infinity);
})();

(function tc_15_08_02_12__013() {
  assert(Math.min.length === 2);
})();

(function tc_15_08_02_12__012() {
  assert(typeof Math.min === "function");
})();

(function tc_15_08_02_12__014() {
  assert(1/Math.min(+0, -0) === -Infinity);
})();

(function tc_15_08_02_12__005() {
  assert(isNaN(Math.min(5, 7, NaN)));
})();

(function tc_15_08_02_12__001() {
  assert(isNaN(Math.min(undefined)));
})();

(function tc_15_08_02_12__002() {
  assert(isNaN(Math.min({})));
})();

(function tc_15_08_02_12__008() {
  assert(!isFinite(Math.min()));
})();

(function tc_15_08_02_12__003() {
  assert(isNaN(Math.min(Object())));
})();

(function tc_15_08_02_12__009() {
  assert(Math.min() === Infinity);
})();

(function tc_15_08_02_12__004() {
  assert(isNaN(Math.min(NaN)));
})();

(function tc_15_08_02_12__007() {
  assert(Math.min(5, -7) === -7);
})();

(function tc_15_08_02_12__006() {
  assert(isNaN(Math.min(5, -7, NaN)));
})();

(function tc_15_08_02_12__011() {
  assert(Math.min(+0, -0) === +0);
})();

(function tc_15_08_02_18__007() {
  assert (isNaN(Math.tan(undefined)));
})();

(function tc_15_08_02_18__001() {
  assert (isNaN(Math.tan(NaN)));
})();

(function tc_15_08_02_18__006() {
  assert (1/Math.tan(-0, NaN) === -Infinity);
})();

(function tc_15_08_02_18__005() {
  assert (1/Math.tan(-0) === -Infinity);
})();

(function tc_15_08_02_18__002() {
  assert (isNaN(Math.tan(Infinity)));
})();

(function tc_15_08_02_18__003() {
  assert (isNaN(Math.tan(-Infinity)));
})();

(function tc_15_08_02_18__004() {
  assert (1/Math.tan(+0) === Infinity);
})();

(function tc_15_08_02_04__002() {
  assert(isNaN(Math.atan(undefined)));
})();

(function tc_15_08_02_04__006() {
  assert(Math.atan(Infinity) === Math.PI / 2);
})();

(function tc_15_08_02_04__008() {
  assert(Math.atan(Number.NEGATIVE_INFINITY) === -Math.PI / 2);
})();

(function tc_15_08_02_04__003() {
  assert(isNaN(Math.atan({})));
})();

(function tc_15_08_02_04__007() {
  assert(Math.atan(Number.POSITIVE_INFINITY) === Math.PI / 2);
})();

(function tc_15_08_02_04__004() {
  assert(Math.atan(+0) === +0);
})();

(function tc_15_08_02_04__001() {
  assert(isNaN(Math.atan(NaN)));
})();

(function tc_15_08_02_04__009() {
  assert(Math.atan(-1) === -Math.PI / 4);
})();

(function tc_15_08_02_04__005() {
  assert(Math.atan(-0) === -0);
})();

(function tc_15_08_02_09__001() {
  assert (isNaN(Math.floor(NaN)));
})();

(function tc_15_08_02_09__006() {
  assert (1/Math.floor(0.2) === Infinity);
})();

(function tc_15_08_02_09__008() {
  assert (Math.floor(1.9) === -Math.ceil(-1.9));
})();

(function tc_15_08_02_09__007() {
  assert (Math.floor(1.2) === -Math.ceil(-1.2));
})();

(function tc_15_08_02_09__002() {
  assert (1/Math.floor(+0) === Infinity);
})();

(function tc_15_08_02_09__003() {
  assert(1/Math.floor(-0) === -Infinity);
})();

(function tc_15_08_02_09__005() {
  assert (Math.floor(-Infinity) === -Infinity);
})();

(function tc_15_08_02_09__004() {
  assert (Math.floor(Infinity) === Infinity);
})();

(function tc_15_08_02_08__004() {
  assert (Math.exp(Infinity) === Infinity);
})();

(function tc_15_08_02_08__002() {
  assert (Math.exp(+0) === 1);
})();

(function tc_15_08_02_08__003() {
  assert(Math.exp(-0) === 1);
})();

(function tc_15_08_02_08__001() {
  assert (isNaN(Math.exp(NaN)));
})();

(function tc_15_08_02_08__006() {
  assert (1/Math.exp(-Infinity) === Infinity);
})();

(function tc_15_08_02_08__005() {
  assert (Math.exp(Infinity) === Infinity);
})();

(function tc_15_06_01_01__005() {
  assert(false === Boolean(+0));
})();

(function tc_15_06_01_01__011() {
  assert(true === Boolean("abcdef"));
})();

(function tc_15_06_01_01__012() {
  assert(true === Boolean({}));
})();

(function tc_15_06_01_01__002() {
  assert(false === Boolean(null));
})();

(function tc_15_06_01_01__007() {
  assert(false === Boolean(NaN));
})();

(function tc_15_06_01_01__001() {
  assert(false === Boolean(undefined));
})();

(function tc_15_06_01_01__004() {
  assert(true === Boolean(true));
})();

(function tc_15_06_01_01__009() {
  assert(true === Boolean(-11111));
})();

(function tc_15_06_01_01__003() {
  assert(false === Boolean(false));
})();

(function tc_15_06_01_01__010() {
  assert(false === Boolean(""));
})();

(function tc_15_06_01_01__006() {
  assert(false === Boolean(-0));
})();

(function tc_15_06_01_01__008() {
  assert(true === Boolean(11111));
})();

(function tc_15_06_02_01__002() {
  var b = new Boolean (true);
  b.x = 1;
  assert (b.x === 1);
})();

(function tc_15_06_02_01__001() {
  assert(Boolean.prototype.isPrototypeOf(new Boolean(true)));
})();

(function tc_15_06_04_03__001() {
  assert(Boolean(false).valueOf() === false);
})();

(function tc_15_06_04_02__003() {
  assert(true.toString() === "true");
})();

(function tc_15_06_04_02__001() {
  assert(Boolean(true).toString() === "true");
})();

(function tc_15_06_04_02__002() {
  assert(Boolean(false).toString() === "false");
})();

(function tc_15_06_04_01__001() {
  assert(Boolean.prototype.constructor === Boolean);
})();

(function tc_15_06_03_01__001() {
  for (var p in Boolean) {
    if (p === Boolean.prototype) {
      assert(false);
    }
  }
})();

(function tc_15_04_02_02__007() {
  var a = new Array("5");
  assert(a.length === 1);
})();

(function tc_15_04_02_02__003() {
  var a = new Array(5);
  assert(a[0] === undefined);
})();

(function tc_15_04_02_02__008() {
  var a = new Array("55");
  assert(a[0] === "55");
})();

(function tc_15_04_02_02__005() {
  var a = new Array(5);
  assert(a[10] === undefined);
})();

(function tc_15_04_02_02__002() {
  var a = new Array(5);
  assert(a.length === 5);
})();

(function tc_15_04_02_02__006() {
  var a = new Array(0);
  assert(a.length === 0);
})();

(function tc_15_04_02_02__004() {
  var a = new Array(5);
  assert(a[3] === undefined);
})();

(function tc_15_04_02_02__001() {
  var a = new Array(5);
  assert(typeof a === 'object');
})();

(function tc_15_04_02_01__001() {
  var a = new Array();
  assert(typeof a === 'object');
})();

(function tc_15_04_02_01__004() {
  var a = new Array(1, 2, 5);
  assert(a[1] === 2);
})();

(function tc_15_04_02_01__003() {
  var a = new Array(1, 2, 5);
  assert(a[0] === 1);
})();

(function tc_15_04_02_01__008() {
  var a = new Array();
  assert(a[0] === undefined);
})();

(function tc_15_04_02_01__005() {
  var a = new Array(1, 2, 5);
  assert(a[2] === 5);
})();

(function tc_15_04_02_01__007() {
  var a = new Array();
  assert(a.length === 0);
})();

(function tc_15_04_02_01__006() {
  var a = new Array(1, 2, 5);
  assert(a[3] === undefined);
})();

(function tc_15_04_02_01__002() {
  var a = new Array(1, 2, 5);
  assert(a.length === 3);
})();

(function tc_15_03_04_02__001() {
  assert(Function.prototype.toString.hasOwnProperty('length'));
  assert(delete Function.prototype.toString.length);
  assert(!Function.prototype.toString.hasOwnProperty('length'));
})();
