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

(function tc_11_10__001() {
  var a = 10;
  a = 1 & 2;
  assert(a == 0)
})();

(function tc_11_10__006() {
  var a;
  a = 4 | 1;
  assert(a == 5)
})();

(function tc_11_10__004() {
  var a = 4;
  var b = null;
  a = 1 & b;
  assert(a == 0)
})();

(function tc_11_10__016() {
  var a = 10;
  a = 1 & 2147483648;
  assert(a == 0)
})();

(function tc_11_10__017() {
  var a = 10;
  a = 1 & 2147483647;
  assert(a == 1)
})();

(function tc_11_10__003() {
  var a = 4;
  var b = "0";
  a = 1 & b;
  assert(a == 0)
})();

(function tc_11_10__008() {
  var a;
  var b = "0";
  a = 1 | b;
  assert(a == 1)
})();

(function tc_11_10__010() {
  var a = 4;
  var b;
  a = 1 | b;
  assert(a == 1)
})();

(function tc_11_10__012() {
  var a = 4;
  var b = 1;
  a ^ b;
  assert(a == 4 && b == 1)
})();

(function tc_11_10__011() {
  var a;
  a = 1 ^ 3;
  assert(a == 2)
})();

(function tc_11_10__015() {
  var a = 4;
  var b;
  a = 1 ^ b;
  assert(a == 1)
})();

(function tc_11_10__014() {
  var a = 4;
  var b = null;
  a = 1 ^ b;
  assert(a == 1)
})();

(function tc_11_10__013() {
  var a;
  var b = "0";
  a = 1 ^ b;
  assert(a == 1)
})();

(function tc_11_10__007() {
  var a = 4;
  var b = 1;
  a | b;
  assert(a == 4 && b == 1)
})();

(function tc_11_10__009() {
  var a = 4;
  var b = null;
  a = 1 | b;
  assert(a == 1)
})();

(function tc_11_10__002() {
  var a = 4;
  var b = 1;
  a & b;
  assert(a == 4 && b == 1)
})();

(function tc_11_10__005() {
  var a = 4;
  var b;
  a = 1 & b;
  assert(a == 0)
})();

(function tc_11_10__018() {
  var a = 10;
  a = 2147483647 & 2147483649;
  assert(a == 1)
})();

(function tc_11_01_06__003() {
  var a = 2;
  var b = 3;

  assert((a) + (b) === (a + b));
})();

(function tc_11_01_06__009() {
  assert(typeof (a) === "undefined");
})();

(function tc_11_01_06__006() {
  a = {
    n: Number,
    s: String
  }

  assert(typeof (a.property) === "undefined");
})();

(function tc_11_01_06__005() {
  a = {
    n: Number,
    s: String
  }

  assert(delete(a.n) === true);
})();

(function tc_11_01_06__004() {
  a = {
    n: Number,
    s: String
  };
  b = {
    n: Number,
    s: String
  };
  a.n = 1;
  b.n = 2;
  a.s = "qwe";
  b.s = "rty";

  assert(((a).n + (b).n === 3) && ((a).s + (b).s === "qwerty"));
})();

(function tc_11_01_06__002() {
  var a = 1;
  var b = 2;
  assert(a + b === (a + b));
})();

(function tc_11_01_06__001() {
  var a = [1, 2, 4];
  var cnt = 0;

  for (var i = (0 in a) ? 1 : 2; i < 10; ++i)
  {
    ++cnt;
  }

  assert(cnt == 9);
})();

(function tc_11_01_05__001() {
  var a = {
    b: 5
  };

  assert(a.b === 5);
})();

(function tc_11_01_05__006() {
  var a = {
    get a() {
      return 3;
    }
  };

  assert(a.a === 3);
})();

(function tc_11_01_05__008() {
  var a = {
    _a: 3,
    get a() {
      return this._a;
    },
    set a(newa) {
      this._a = newa;
    }

  };

  a.a = 5;

  assert(a.a === 5);
})();

(function tc_11_01_05__002() {
  var a = {
    "b": 5
  };

  assert(a.b === 5);
})();

(function tc_11_01_05__004() {
  var a = {
    10.25: 5
  };

  assert(a[10.25] === 5);
})();

(function tc_11_01_05__003() {
  var a = {
    10: 5
  };

  assert(a[10] === 5);
})();

(function tc_11_01_05__007() {
  var a = {
    _a: 3,
    get a() {
      return this._a;
    }
  };

  a._a = 5;

  assert(a.a === 5);
})();

(function tc_11_01_05__005() {
  var a = {
    prop1: 1,
    prop2: 2
  };

  assert(a.prop1 === 1 && a.prop2 === 2);
})();

(function tc_11_13_02__013() {
  var a = 0xffffffff;
  var _a = a;
  var b = 4;
  assert ((a <<= b) === (_a << b));
})();

(function tc_11_13_02__047() {
  object = {
    valueOf: function () {
      return 16
    },
    toString: function () {
      return 0
    }
  }

  copyObject = object;
  assert(((object >>>= 2) === (copyObject >>> 2)) && (object === (copyObject >>> 2)))
})();

(function tc_11_13_02__005() {
  var a = true;
  var b = false;
  a += b;
  assert(a === 1)
})();

(function tc_11_13_02__039() {
  var a = 4;
  var _a = a;
  var b = 10;
  assert((a %= b) === (_a % b))
})();

(function tc_11_13_02__040() {
  object = {
    valueOf: function () {
      return 1
    },
    toString: function () {
      return 0
    }
  }

  copyObject = object;
  assert(((object *= 2) === (copyObject * 2)) && (object === (copyObject * 2)))
})();

(function tc_11_13_02__050() {
  object = {
    valueOf: function () {
      return 1
    },
    toString: function () {
      return 0
    }
  }

  copyObject = object;
  assert(((object ^= 1) === (copyObject ^ 1)) && (object === (copyObject ^ 1)))
})();

(function tc_11_13_02__008() {
  var a = 5;
  var _a = a;
  var b = 10;
  assert((a -= b) === (_a - b))
})();

(function tc_11_13_02__002() {
  var a = 1;
  var b = "2";
  a += b;
  assert(a === "12")
})();

(function tc_11_13_02__011() {
  var a = 102;
  var _a = a;
  var b = 10;
  assert(((a %= b) === (_a % b)) && (a === (_a % b)))
})();

(function tc_11_13_02__014() {
  var a = 0xffffffff;
  var _a = a;
  var b = 4;
  assert ((a >>>= b) === (_a >>> b));
})();

(function tc_11_13_02__001() {
  var a = 1;
  var b = 2;
  a += b;
  assert(a === 3);
})();

(function tc_11_13_02__003() {
  var a = "1";
  var b = 2;
  a += b;
  assert(a === "12")
})();

(function tc_11_13_02__043() {
  object = {
    valueOf: function () {
      return 178
    },
    toString: function () {
      return 0
    }
  }

  copyObject = object;
  assert(((object /= 2) === (copyObject / 2)) && (object === (copyObject / 2)))
})();

(function tc_11_13_02__007() {
  var a = 3;
  var _a = a;
  var b = 7;
  assert((a += b) === (_a + b))
})();

(function tc_11_13_02__045() {
  object = {
    valueOf: function () {
      return 16
    },
    toString: function () {
      return 0
    }
  }

  copyObject = object;
  assert(((object <<= 2) === (copyObject << 2)) && (object === (copyObject << 2)))
})();

(function tc_11_13_02__010() {
  var a = 1;
  var _a = a;
  var b = 10.6;
  assert((a /= b) === (_a / b))
})();

(function tc_11_13_02__046() {
  object = {
    valueOf: function () {
      return 16
    },
    toString: function () {
      return 0
    }
  }

  copyObject = object;
  assert(((object >>= 2) === (copyObject >> 2)) && (object === (copyObject >> 2)))
})();

(function tc_11_13_02__042() {
  object = {
    valueOf: function () {
      return 15
    },
    toString: function () {
      return ""
    }
  }

  copyObject = object;
  assert(((object -= 2) === (copyObject - 2)) && (object === (copyObject - 2)))
})();

(function tc_11_13_02__004() {
  var a = "1";
  var b = "2";
  a += b;
  assert(a === "12")
})();

(function tc_11_13_02__049() {
  object = {
    valueOf: function () {
      return 1
    },
    toString: function () {
      return 0
    }
  }

  copyObject = object;
  assert(((object |= 1) === (copyObject | 1)) && (object === (copyObject | 1)))
})();

(function tc_11_13_02__009() {
  var a = 10;
  var _a = a;
  var b = 1.5;
  assert((a *= b) === (_a * b))
})();

(function tc_11_13_02__044() {
  object = {
    valueOf: function () {
      return 1345
    },
    toString: function () {
      return "foo"
    }
  }

  copyObject = object;
  assert(((object %= 2) === (copyObject % 2)) && (object === (copyObject % 2)))
})();

(function tc_11_13_02__048() {
  object = {
    valueOf: function () {
      return 1
    },
    toString: function () {
      return 0
    }
  }

  copyObject = object;
  assert(((object &= 1) === (copyObject & 1)) && (object === (copyObject & 1)))
})();

(function tc_11_13_02__006() {
  var a = 1;
  var b = null;
  a += b;
  assert(a === 1)
})();

(function tc_11_13_02__041() {
  object = {
    valueOf: function () {
      return 1
    },
    toString: function () {
      return 0
    }
  }

  copyObject = object;
  assert(((object += 2) === (copyObject + 2)) && (object === (copyObject + 2)))
})();

(function tc_11_13_02__051() {
  object = {
    valueOf: function () {
      return "12"
    },
    toString: function () {
      return 0
    }
  }

  copyObject = object;
  assert(((object += 3) === (copyObject + 3)) && (object === (copyObject + 3)))
})();

(function tc_11_13_02__012() {
  var a = 0xffffffff;
  var _a = a;
  var b = 4;
  assert ((a >>= b) === (_a >> b));
})();

(function tc_11_13_01__001() {
  var a = 5;
  var b;

  b = a;
  assert(b == 5)
})();

(function tc_11_07_01__008() {
  var a = 5;
  var b = a << true;
  assert(b == 10)
})();

(function tc_11_07_01__006() {
  var a = null;
  var b = a << 2;
  assert(b == 0)
})();

(function tc_11_07_01__004() {
  var a = 5;
  var b = a << "2";
  assert(b == 20)
})();

(function tc_11_07_01__003() {
  var a = 5;
  var b = a << 1 + 1;
  assert(b == 20)
})();

(function tc_11_07_01__007() {
  var a = 5;
  var b = a << null;
  assert(b == 5)
})();

(function tc_11_07_01__005() {
  var a;
  var b = a << 2;
  assert(b == 0)
})();

(function tc_11_07_01__001() {
  var a = 5;
  var b = a << 2
  assert(b == 20)
})();

(function tc_11_07_01__009() {
  var a = 5;
  var b = a << -1;
  assert(b == -2147483648)
})();

(function tc_11_07_03__007() {
  var a = 20;
  var b = a >>> true;
  assert(b == 10)
})();

(function tc_11_07_03__006() {
  var a = 5;
  var b = a >>> null;
  assert(b == 5)
})();

(function tc_11_07_03__004() {
  var a;
  var b = a >>> 2;
  assert(b == 0)
})();

(function tc_11_07_03__005() {
  var a = null;
  var b = a >>> 2;
  assert(b == 0)
})();

(function tc_11_07_03__002() {
  var a = 20;
      var b = a >>> 1+1;
  assert(b == 5)
})();

(function tc_11_07_03__003() {
  var a = 20;
  var b = a >>> "2";
  assert(b == 5)
})();

(function tc_11_07_03__001() {
  var a = 20;
  var b = a >>> 2
  assert(b == 5)
})();

(function tc_11_07_02__001() {
  var a = 20;
  var b = a >> 2
  assert(b == 5)
})();

(function tc_11_07_02__003() {
  var a = 20;
  var b = a >> "2";
  assert(b == 5)
})();

(function tc_11_07_02__004() {
  var a;
  var b = a >> 2;
  assert(b == 0)
})();

(function tc_11_07_02__005() {
  var a = null;
  var b = a >> 2;
  assert(b == 0)
})();

(function tc_11_07_02__009() {
  var b = -2147483648 >> 30;
  assert(b == -2)
})();

(function tc_11_07_02__006() {
  var a = 5;
  var b = a >> null;
  assert(b == 5)
})();

(function tc_11_07_02__007() {
  var a = 20;
  var b = a >> true;
  assert(b == 10)
})();

(function tc_11_07_02__002() {
  var a = 20;
  var b = a >> 1 + 1;
  assert(b == 5)
})();

(function tc_11_07_02__008() {
  var a = 5;
  var b = a >> -1;
  assert(b == 0)
})();

(function tc_11_02_01__004() {
  var name = "name"
  var a = {name: "name", value: "1"};

  assert(a[name] == "name");
})();

(function tc_11_02_01__002() {
  var a = {name: "name", value: "1"};
  var b = {name: "b", value: "1"};

  assert((a.name == b.name) || (a.value == b.value));
})();

(function tc_11_02_01__011() {
  var a = {name: "a", value: 1};
  var b = {name: "b", value: 1};

  assert(plus(a, b) === 2);

  function plus(a, b)
  {
    return a.value + b.value;
  }
})();

(function tc_11_02_01__003() {
  var a = {name: "name", value: "1"};

  assert(a[1] !== "nameeeeeeeeeee");
})();

(function tc_11_02_01__008() {
  var a = {name: "a", value: "1"};
  var b = {name: "b", value: "1"};

  assert(isNaN(plus(a, b)));

  function plus(a, b)
  {
    return a.value * b.name;
  }
})();

(function tc_11_02_01__010() {
  var a = {name: "a", value: "1"};
  var b = {name: "b", value: 1};

  assert(plus(a, b) === "11");

  function plus(a, b)
  {
    return a.value + b.value;
  }
})();

(function tc_11_02_01__001() {
  var a = {name: "name", value: "1"};

  assert(a.name !== "nameeeeeeeeeee");
})();

(function tc_11_02_01__007() {
  var a = {name: "a", value: "1"};
  var b = {name: "b", value: "1"};
  assert(plus(a, b) !== 2)

  function plus(a, b)
  {
    return a.value + b.value;
  }
})();

(function tc_11_02_01__009() {
  var a = {name: "a", value: "1"};
  var b = {name: "b", value: "1"};

  assert(plus(a, b) === "11");

  function plus(a, b)
  {
    return a.value + b.value;
  }
})();

(function tc_11_02_04__003() {
  assert(f_arg().length === 0);

  function f_arg(x, y) {
    return arguments;
  }
})();

(function tc_11_02_04__010() {
  f_arg = function () {
    return arguments;
  }

  var args = f_arg (1, 2, 3);

  for (var i = 0; i < 3; i++)
  {
    assert(args[i] === i + 1);
  }

  assert(args[3] === undefined);
})();

(function tc_11_02_04__012() {
  f_arg = function(x,y) {
    return arguments;
  }

  assert(f_arg(1)[0] === 1);
})();

(function tc_11_02_04__007() {
  f_arg = function () {
    return arguments;
  }

  assert(f_arg(1, 2, 3)[0] === 1);
})();

(function tc_11_02_04__017() {
  function f_arg() {
  }

  try
  {
    f_arg(x, x = 1);
  }
  catch (e) {
    assert((e instanceof ReferenceError) === true);
  }
})();

(function tc_11_02_04__019() {
  function f_arg() {
  }


  var x = function () {
    throw "x";
  };
  var y = function () {
    throw "y";
  };
  try
  {
    f_arg(x(), y());
    assert(false);
  }
  catch (e)
  {
    if (e === "y")
    {
      assert(false);
    } else {
      if (e !== "x")
      {
        assert(false);
      }
    }
  }
})();

(function tc_11_02_04__016() {
  function f_arg() {
  }

  f_arg(x=1,x);
})();

(function tc_11_02_04__014() {
  f_arg = function(x,y) {
    return arguments;
  }

  assert(f_arg(1,2,3)[3] === undefined);
})();

(function tc_11_02_04__006() {
  f_arg = function () {
    return arguments;
  }

  assert(f_arg(1, 2, 3).length === 3);
})();

(function tc_11_02_04__004() {
  assert(f_arg()[0] === undefined);

  function f_arg(x,y) {
    return arguments;
  }
})();

(function tc_11_02_04__011() {
  f_arg = function(x,y) {
    return arguments;
  }

  assert(f_arg(1,2,3).length === 3);
})();

(function tc_11_02_04__001() {
  assert(f_arg().length === 0);

  function f_arg() {
    return arguments;
  }
})();

(function tc_11_02_04__013() {
  f_arg = function (x, y) {
    return arguments;
  }

  assert(f_arg(1, 2, 3)[2] === 3);
})();

(function tc_11_02_04__009() {
  f_arg = function () {
    return arguments;
  }

  assert(f_arg(1, 2, 3)[3] === undefined);
})();

(function tc_11_02_04__005() {
  f_arg = function ()
  {
    return arguments
  }

  assert(f_arg(1, 2).length === 2);

  f_arg = function () {
    return arguments;
  }
})();

(function tc_11_02_04__002() {
  assert(f_arg()[0] === undefined);

  function f_arg() {
    return arguments;
  }
})();

(function tc_11_02_04__008() {
  f_arg = function () {
    return arguments;
  }

  assert(f_arg(1, 2, 3)[2] === 3);
})();

(function tc_11_02_04__018() {
  function f_arg(x, y, z) {
    return z;
  }

  assert(f_arg(x = 1, y = x, x + y) === 2);
})();

(function tc_11_02_02__006() {
  function Animal(name)
  {
    this.name = name
    this.canWalk = true
    if (name == "bird")
    {
      this.canFly = true;
    }
  }

  var animal = new Animal("animal");
  var bird = new Animal("bird");
  assert(animal.canFly !== bird.canFly);
})();

(function tc_11_02_02__004() {
  function Animal(name)
  {
    this.name = name
    this.canWalk = true
  }

  var animal = new Animal("animal");
  assert(animal.someparameter != "insect");
})();

(function tc_11_02_02__001() {
  function Animal(name)
  {
    this.name = name
    this.canWalk = true
  }

  var animal = new Animal("animal");
  assert(animal.name === "animal");
})();

(function tc_11_02_02__007() {
  function Animal(name)
  {
    this.name = name
    this.canWalk = true
    if (name == "bird")
    {
      this.canFly = true;
    }
  }

  var animal = new Animal("animal");
  var bird = new Animal("bird");

  assert(animal.canWalk === true);
  assert(bird.canWalk === true);
  assert(animal.canFly === undefined);
  assert(bird.canFly === true);
})();

(function tc_11_02_02__009() {
  var a = {};
  a.b = true;
  assert(typeof a == "object" && a.b == 1);
})();

(function tc_11_02_02__002() {
  function Animal(name)
  {
    this.name = name
    this.canWalk = true
  }

  var animal = new Animal("animal");
  assert(animal.name == "animal");
})();

(function tc_11_02_02__003() {
  function Animal(name)
  {
    this.name = name
    this.canWalk = true
  }

  var animal = new Animal("animal");
  assert(animal.name != "insect");
})();

(function tc_11_02_02__005() {
  function Animal(name)
  {
    this.name = name
    this.canWalk = true
  }

  var animal = new Animal("animal");
  assert(animal[1] != "animal");
})();

(function tc_11_02_02__008() {
  function Animal(name)
  {
    this.name = name
    this.canWalk = true
  }

  var animal = new Animal();

  assert(animal.canWalk);
})();

(function tc_11_02_03__007() {
  function foo()
  {
    return 1;
  }
  assert(foo() === 1);
})();

(function tc_11_02_03__006() {
  var a = {};
  a.toString();
})();

(function tc_11_02_03__017() {
  var obj = {
    field: Number,
    foo: function () {
      this.field++;
    }
  }

  obj.field = 3;
  obj.foo();

  assert(obj.field === 4);
})();

(function tc_11_02_03__021() {
  var a = 1;
  var b = foo();
  function foo()
  {
    return a;
  }

  assert(b === 1);
})();

(function tc_11_02_03__008() {
  var a = {
    foo: function ()
    {
      return 1;
    }
  }

  assert(a.foo() === 1);
})();

(function tc_11_14__002() {
  var a, b, c, res;

  res = (a = 39, b = null, c = 12.5);

  assert(a == 39 && b == null && c == 12.5 && res == 12.5)
})();

(function tc_11_14__001() {
  var res = (33, false, 73.234, 100);

  assert(res == 100);
})();

(function tc_11_04_07__001() {
  var a = 1;
  assert(-a === -1)
})();

(function tc_11_04_07__014() {
  var a = new Number(1);
  assert(-a === -1)
})();

(function tc_11_04_07__009() {
  var a = false;
  assert(-a === 0)
})();

(function tc_11_04_07__024() {
  var a = {
    valueOf: function () {
      return -1;
    }
  }
  assert(-a === 1)
})();

(function tc_11_04_07__010() {
  var a = Infinity;
  assert(-a === -Infinity)
})();

(function tc_11_04_07__011() {
  var a = -Infinity;
  assert(-a === Infinity)
})();

(function tc_11_04_07__029() {
  var a = {
    member: 1,
    valueOf: function () {
      return this.member;
    }
  }

  assert(-a === -1)
})();

(function tc_11_04_07__004() {
  var a = "-1";
  assert(-a === 1)
})();

(function tc_11_04_07__003() {
  var a = "1";
  assert(-a === -1)
})();

(function tc_11_04_07__002() {
  var a = -1;
  assert(-a === 1)
})();

(function tc_11_04_07__006() {
  var a = "0";
  assert(-a === 0)
})();

(function tc_11_04_07__022() {
  var a = [1, 2, 3, 4, 5];
  assert(isNaN(-a))
})();

(function tc_11_04_07__028() {
  var a = {
    valueOf: function () {
      return undefined;
    }
  }
  assert(isNaN(-a))
})();

(function tc_11_04_07__027() {
  var a = {
    valueOf: function () {
      return null;
    }
  }
  assert(-a === -0)
})();

(function tc_11_04_07__008() {
  var a = true;
  assert(-a === -1)
})();

(function tc_11_04_07__018() {
  var a = new String("-1");
  assert(-a === 1)
})();

(function tc_11_04_07__012() {
  var a = undefined;
  assert(isNaN(-a))
})();

(function tc_11_04_07__016() {
  var a = new Number(0);
  assert(-a === 0)
})();

(function tc_11_04_07__017() {
  var a = new String("1");
  assert(-a === -1)
})();

(function tc_11_04_07__019() {
  var a = new String("");
  assert(-a === 0)
})();

(function tc_11_04_07__021() {
  var a = new Boolean(false);
  assert(-a === 0)
})();

(function tc_11_04_07__025() {
  var a = {
    valueOf: function () {
      return true;
    }
  }
  assert(-a === -1)
})();

(function tc_11_04_07__030() {
  var a = {
    member: 1,
  }

  assert(isNaN(-a))
})();

(function tc_11_04_07__005() {
  var a = 0;
  assert(-a === 0)
})();

(function tc_11_04_07__007() {
  var a = "";
  assert(-a === 0)
})();

(function tc_11_04_07__026() {
  var a = {
    valueOf: function () {
      return false;
    }
  }
  assert(-a === 0)
})();

(function tc_11_04_07__023() {
  var a = {
    valueOf: function () {
      return 1;
    }
  }
  assert(-a === -1)
})();

(function tc_11_04_07__013() {
  var a = null;
  assert(-a === -0)
})();

(function tc_11_04_07__031() {
  var a = {
    valueOf: function () {
      return "qwerty";
    }
  }

  assert(isNaN(-a))
})();

(function tc_11_04_07__015() {
  var a = new Number(-1);
  assert(-a === 1)
})();

(function tc_11_04_07__033() {
  var a = new Object;
  a.toString = function () {
    return "1";
  }

  assert(-a === -1)
})();

(function tc_11_04_07__032() {
  var a = +0;

  assert(-a === -0)
})();

(function tc_11_04_07__020() {
  var a = new Boolean(true);
  assert(-a === -1)
})();

(function tc_11_04_02__002() {
  var b = 1;

  var a = void(++b);

  assert(a == undefined && b == 2);
})();

(function tc_11_04_02__001() {
  var a = void(5 / 2);

  assert(a == undefined);
})();

(function tc_11_04_04__005() {
  var a = 25;

  assert(++
          a === 26)
})();

(function tc_11_04_04__011() {
  var a = "abc";

  assert(isNaN(++a));
})();

(function tc_11_04_04__001() {
  var a = 25;

  assert(++a === 26);
})();

(function tc_11_04_04__006() {
  var a = 1.12;
  var eps = 0.00000001;

  assert(++a >= 2.12 - eps && a <= 2.12 + eps);
})();

(function tc_11_04_04__009() {
  var a = undefined;

  assert(isNaN(++a))
})();

(function tc_11_04_04__002() {
  var a = 25, b = -1;
  ;

  assert(++a === ++b + 26);
})();

(function tc_11_04_04__012() {
  var a = function () {
  };

  assert(isNaN(++a));
})();

(function tc_11_04_04__007() {
  var a = true;

  assert(++a === 2);
})();

(function tc_11_04_04__010() {
  var a = null;

  assert(++a === 1);
})();

(function tc_11_04_04__008() {
  var a = {};

  assert(isNaN(++a))
})();

(function tc_11_04_04__004() {
  var a = 25;

  assert((++a) / 2 === 13);
})();

(function tc_11_04_01__013() {
  eval('var foo = 1;');
  assert((delete foo) == true && typeof foo == "undefined");
})();

(function tc_11_04_01__012() {
  this.test = function (arg)
  {
    return 1;
  }

  assert((delete test) == true);
})();

(function tc_11_04_01__011() {
  function test(arg)
  {
    if ((delete arg) == false)
      return 0;
    else
      return 1;
  }

  assert(!test("str"));
})();

(function tc_11_04_01__002() {
  var y = 43;

  assert((delete y) == false && y == 43);
})();

(function tc_11_04_01__017() {
  assert((delete i_dont_exist) == true);
})();

(function tc_11_04_01__004() {
  var myobj = {
    h: 4,
    k: 5
  };

  assert((delete myobj.h) == true && myobj.h == undefined);
})();

(function tc_11_04_01__007() {
  var fruits = ['apple', 'banana', 'kiwi', 'pineapple'];

  delete fruits[3];

  assert(!(3 in fruits) && fruits.length == 4);
})();

assert((delete arguments) == true);

(function tc_11_04_01__006() {
  function Foo() {
  }
  Foo.prototype.bar = 42;
  var foo = new Foo();
  if (!(delete foo.bar))
    assert(false)

  if (foo.bar != 42)
    assert(false)

  if (!(delete Foo.prototype.bar))
    assert(false)
})();

(function tc_11_04_01__003() {
  var y = 43;

  assert((delete Math.PI) == false);
})();

(function tc_11_04_01__008() {
  function x() {
  }

  assert((delete x) == false && typeof x == "function");
})();

(function tc_11_04_01__009() {
  this.prop = "prop";

  assert((delete this.prop) == true);
})();

(function tc_11_04_01__005() {
  myobj = {
          h: 4,
          k: 5
      };

  assert ((delete myobj) == true);
})();

(function tc_11_04_01__001() {
  x = 42;

  assert ((delete x) == true);
})();

(function tc_11_04_06__013() {
  var a = new String("-1");
  assert(+a === -1)
})();

(function tc_11_04_06__008() {
  var a = new Number(1);
  assert(+a === 1)
})();

(function tc_11_04_06__017() {
  var a = new String("qwerty");
  assert(isNaN(+a))
})();

(function tc_11_04_06__018() {
  var a = {
    valueOf: function () {
      return 1;
    }
  }

  assert(+a === 1)
})();

(function tc_11_04_06__014() {
  var a = undefined;
  assert(isNaN(+a))
})();

(function tc_11_04_06__024() {
  var a = {
    member: Number
  }

  assert(isNaN(+a))
})();

(function tc_11_04_06__026() {
  var array = [1, 2, 3, 4, 5];
  assert(isNaN(+array))
})();

(function tc_11_04_06__004() {
  var a = "-1";
  assert(+a === -1)
})();

(function tc_11_04_06__001() {
  var a = 1;
  assert(+a === a)
})();

(function tc_11_04_06__005() {
  var a = true;
  assert(+a === 1)
})();

(function tc_11_04_06__023() {
  var a = {
    valueOf: function () {
      return "not a number";
    }
  }

  assert(isNaN(+a))
})();

(function tc_11_04_06__007() {
  var a = new Number(0);
  assert(+a === 0);
})();

(function tc_11_04_06__012() {
  var a = new String("1");
  assert(+a === 1)
})();

(function tc_11_04_06__022() {
  var a = {
    valueOf: function () {
      return false;
    }
  }

  assert(+a === +0)
})();

(function tc_11_04_06__010() {
  var a = new Boolean(true);
  assert(+a === 1)
})();

(function tc_11_04_06__006() {
  var a = false;
  assert(+a === 0)
})();

(function tc_11_04_06__020() {
  var a = {
    valueOf: function () {
      return -1;
    }
  }

  assert(+a === -1)
})();

(function tc_11_04_06__021() {
  var a = new Object;
  a.valueOf = function () {
    return true;
  }

  assert(+a === 1)
})();

(function tc_11_04_06__002() {
  var a = -1;
  assert(+a === a)
})();

(function tc_11_04_06__027() {
  var a = {
    toString: function () {
      return "1"
    }
  }

  assert(+a === 1)
})();

(function tc_11_04_06__015() {
  var a = null;
  assert(+a === +0)
})();

(function tc_11_04_06__025() {
  var a = {
    valueOf: function () {
      return null;
    }
  }
  assert(+a === +0)
})();

(function tc_11_04_06__019() {
  var a = {
    valueOf: function () {
      return "1";
    }
  }

  assert(+a === 1)
})();

(function tc_11_04_06__028() {
  var a = {
    valueOf: function () {
      return ""
    }
  }

  assert(+a === 0)
})();

(function tc_11_04_06__009() {
  var a = new Number(-1);
  assert(+a === -1)
})();

(function tc_11_04_06__003() {
  var a = "1";
  assert(+a === 1)
})();

(function tc_11_04_06__011() {
  var a = new Boolean(false);
  assert(+a === +0)
})();

(function tc_11_04_06__016() {
  var a = NaN;
  assert(isNaN(+a))
})();

(function tc_11_04_09__006() {
  assert(!(+0) === true)
})();

(function tc_11_04_09__015() {
  assert(![] === false)
})();

(function tc_11_04_09__004() {
  var a = null;
  assert(!a === true)
})();

(function tc_11_04_09__017() {
  assert(!1 === false)
})();

(function tc_11_04_09__013() {
  assert(!true === false)
})();

(function tc_11_04_09__005() {
  var a = null;
  assert(!a === true)
})();

(function tc_11_04_09__012() {
  var a = {
    valueOf: function () {
      return false;
    }
  }
  assert(!a === false)
})();

(function tc_11_04_09__002() {
  var a = false;
  assert(!a === true)
})();

(function tc_11_04_09__010() {
  assert(!("anything") === false)
})();

(function tc_11_04_09__011() {
  var a = new Object;
  assert(!a === false)
})();

(function tc_11_04_09__001() {
  var a = true;
  assert(!a === false)
})();

(function tc_11_04_09__007() {
  assert(!(-0) === true)
})();

(function tc_11_04_09__018() {
  assert(!(-Infinity) === false)
})();

(function tc_11_04_09__014() {
  assert(!false === true)
})();

(function tc_11_04_09__016() {
  assert(!0 === true)
})();

(function tc_11_04_09__003() {
  var a = undefined;
  assert(!a === true)
})();

(function tc_11_04_09__008() {
  assert(!(NaN) === true)
})();

(function tc_11_04_09__019() {
  assert(!Infinity === false)
})();

(function tc_11_04_09__009() {
  assert(!("") === true)
})();

(function tc_11_04_09__020() {
  var a = new Boolean(true);
  assert(!a === false)
})();

(function tc_11_04_08__001() {
  var a = 0;
  assert(~a === -1);
})();

(function tc_11_04_08__019() {
  var a = {
    valueOf: function () {
      return "0x001"
    }
  }
  assert(~a === -0x002)
})();

(function tc_11_04_08__007() {
  var a = -0;
  assert(~a === -1)
})();

(function tc_11_04_08__020() {
  var a = {
    valueOf: function () {
      return -0x01
    }
  }
  assert(~a === 0)
})();

(function tc_11_04_08__011() {
  var a = 0x1fffffffe;
  assert(~a === ~(0xfffffffe))
})();

(function tc_11_04_08__022() {
  var a = Number(1);
  assert(~a === -2)
})();

(function tc_11_04_08__021() {
  var a = {
    valueOf: function () {
      return true
    }
  }
  assert(~a === -2)
})();

(function tc_11_04_08__002() {
  var a = -1;
  assert(~a === 0)
})();

(function tc_11_04_08__006() {
  var a = +0;
  assert(~a === -1)
})();

(function tc_11_04_08__018() {
  var a = false;
  assert(~a === -1)
})();

(function tc_11_04_08__016() {
  var a = "Who cares?";
  assert(~a === -1)
})();

(function tc_11_04_08__003() {
  var a = 1;
  assert(~a === -2)
})();

(function tc_11_04_08__014() {
  var a = 0xffff;
  assert(~a === -0x10000)
})();

(function tc_11_04_08__008() {
  var a = +Infinity;
  assert(~a === -1)
})();

(function tc_11_04_08__017() {
  var a = true;
  assert(~a === -2)
})();

(function tc_11_04_08__004() {
  var a = 0x0001;
  assert(~a === -0x0002)
})();

(function tc_11_04_08__012() {
  var a = 0x110000000;
  assert(~a === ~(0x10000000))
})();

(function tc_11_04_08__009() {
  var a = -Infinity;
  assert(~a === -1)
})();

(function tc_11_04_08__005() {
  var a = NaN;
  assert(~a === -1)
})();

(function tc_11_04_08__013() {
  var a = 0x1fffffff;
  assert(~a === ~(0x1fffffff))
})();

(function tc_11_04_08__010() {
  var a = 2 * 0x100000000;
  assert(~a === -1)
})();

(function tc_11_04_08__015() {
  var a = "1";
  assert(~a === -2)
})();

(function tc_11_04_05__005() {
  var a = 25;

  assert(--
          a === 24)
})();

(function tc_11_04_05__012() {
  var a = function () {
  };

  assert(isNaN(--a));
})();

(function tc_11_04_05__008() {
  var a = {};

  assert(isNaN(--a))
})();

(function tc_11_04_05__009() {
  var a = undefined;

  assert(isNaN(--a))
})();

(function tc_11_04_05__002() {
  var a = 25, b = 1;
  ;

  assert(--a === --b + 24)
})();

(function tc_11_04_05__006() {
  var eps = 0.000000001;
  var a = 1.12;

  assert(--a >= 0.12 - eps &&
          a <= 0.12 + eps)
})();

(function tc_11_04_05__010() {
  var a = null;

  assert(--a === -1);
})();

(function tc_11_04_05__011() {
  var a = "abc";

  assert(isNaN(--a));
})();

(function tc_11_04_05__004() {
  var a = 25;

  assert((--a) / 2 === 12)
})();

(function tc_11_04_05__007() {
  var a = true;

  assert(--a === 0)
})();

(function tc_11_04_05__001() {
  var a = 25;

  assert(--a === 24)
})();

(function tc_11_04_03__003() {
  assert(typeof Math.LN2 === 'number' &&
          typeof Infinity === 'number' &&
          typeof NaN === 'number' &&
          typeof Number(1) === 'number');
})();

(function tc_11_04_03__012() {
  assert (typeof Math.sin === 'function');
})();

(function tc_11_04_03__007() {
  assert(typeof Boolean(true) === 'boolean');
})();

(function tc_11_04_03__005() {
  assert(typeof (typeof 1) === 'string' &&
          typeof String("str") === 'string');
})();

(function tc_11_04_03__008() {
  assert(typeof undefined === 'undefined' &&
          typeof smth === 'undefined');
})();

(function tc_11_04_03__009() {
  assert(typeof {a: 1} === 'object' &&
          typeof [1, 2, 4] === 'object')
})();

(function tc_11_04_03__006() {
  assert(typeof true === 'boolean' &&
          typeof false === 'boolean');
})();

(function tc_11_04_03__004() {
  assert(typeof "" === 'string' &&
          typeof "str" === 'string');
})();

(function tc_11_04_03__013() {
  assert(typeof null === 'object');
})();

(function tc_11_04_03__011() {
  assert (typeof function(){} === 'function')
})();

(function tc_11_04_03__002() {
  assert(typeof 37 === 'number' &&
          typeof 3.14 === 'number');
})();

(function tc_11_04_03__016() {
  assert(typeof
          24 === 'number')
})();

(function tc_11_04_03__010() {
  assert(typeof new Date() === 'object' &&
          typeof new Boolean(true) === 'object' &&
          typeof new Number(1) === 'object' &&
          typeof new String("abc") === 'object')
})();

(function tc_11_04_03__001() {
  assert(typeof 37 === 'number');
})();

(function tc_11_06_02__006() {
  assert(1 - 1 === 0)
})();

(function tc_11_06_02__013() {
  assert("1" - "1" === 0)
})();

(function tc_11_06_02__012() {
  assert(new Number(1) - new Number(1) === 0)
})();

(function tc_11_06_02__015() {
  assert(new String("1") - new String("1") === 0)
})();

(function tc_11_06_02__002() {
  var a = -10;
  var b = 50;
  assert((a - b === -60) && (b - a === 60))
})();

(function tc_11_06_02__014() {
  assert(!((("1" - new String("1") !== 0) || (new String("1") - 1 !== 0))))
})();

(function tc_11_06_02__004() {
  var a = "string";
  var b = 10;
  assert(isNaN(a - b) && isNaN(b - a))
})();

(function tc_11_06_02__003() {
  var a = -5;
  var b = -100;
  assert(a - b === 95)
})();

(function tc_11_06_02__010() {
  assert(true - true === 0)
})();

(function tc_11_06_02__008() {
  object = {
    valueOf: function () {
      return 1;
    },
    toString: function () {
      return 0;
    }
  }
  assert(!((object - 1 !== 0) || (1 - object !== 0)));
})();

(function tc_11_06_02__009() {
  var x = 0;
  assert(x - (x = 1) === -1)
})();

(function tc_11_06_02__017() {
  assert(isNaN("x" - 1) && isNaN(1 - "x"))
})();

(function tc_11_06_02__001() {
  var a = 100;
  var b = 20;
  assert(a - b === 80)
})();

(function tc_11_06_02__011() {
  assert(!(((new Number(1) - 1 !== 0) || (1 - new Number(1) !== 0))))
})();

(function tc_11_06_02__007() {
  var object1 = new Object();
  var object2 = new Object();
  object1.prop = 1;
  object2.prop = 1;
  assert(object1.prop - object2.prop === 0)
})();

(function tc_11_06_02__005() {
  var x = 1;
  assert(!((x - 1 !== 0) || (1 - x !== 0)))
})();

(function tc_11_06_02__016() {
  assert(isNaN("x" - "1") && isNaN("1" - "x"))
})();

(function tc_11_06_03__024() {
  assert("1" + 1 + 1 === ("1" + 1) + 1)
})();

(function tc_11_06_03__002() {
  obj = new Object;
  assert(isNaN(obj + NaN) && isNaN(NaN + obj))
})();

(function tc_11_06_03__007() {
  assert(-0 + -0 === -0)
})();

(function tc_11_06_03__010() {
  assert(2 + -2 === +0)
})();

(function tc_11_06_03__016() {
  var a = 1;
  var b = -1;
  assert(a - b === a + -b)
})();

(function tc_11_06_03__017() {
  assert(Number.MAX_VALUE - -0 === Number.MAX_VALUE)
})();

(function tc_11_06_03__012() {
  assert(-Number.MAX_VALUE - Number.MAX_VALUE === -Infinity)
})();

(function tc_11_06_03__013() {
  assert(!(Number.MIN_VALUE + -Number.MIN_VALUE !== +0))
})();

(function tc_11_06_03__009() {
  assert(0 + 5 === 5)
})();

(function tc_11_06_03__015() {
  assert(Number.MAX_VALUE - -Number.MAX_VALUE === +Infinity)
})();

(function tc_11_06_03__011() {
  assert((+0 + +0 === +0) && (+0 + -0 === +0))
})();

(function tc_11_06_03__001() {
  var a = 1;
  var b = 2;
  assert(a + b === b + a)
})();

(function tc_11_06_03__008() {
  assert((+0 + +0 === +0) && (+0 + -0 === +0))
})();

(function tc_11_06_03__022() {
  assert(-Number.MAX_VALUE + Number.MAX_VALUE + Number.MAX_VALUE === (-Number.MAX_VALUE + Number.MAX_VALUE) + Number.MAX_VALUE)
})();

(function tc_11_06_03__003() {
  obj = new Object();
  assert(isNaN(obj - NaN) && isNaN(NaN - obj))
})();

(function tc_11_06_03__018() {
  assert(0 - 1 === -1)
})();

(function tc_11_06_03__019() {
  assert(-Number.MAX_VALUE - -Number.MAX_VALUE === +0)
})();

(function tc_11_06_03__021() {
  assert(-8.99e+307 - 8.99e+307 === -Infinity)
})();

(function tc_11_06_03__004() {
  assert(isNaN(Infinity + -Infinity))
})();

(function tc_11_06_03__023() {
  assert((-Number.MAX_VALUE + Number.MAX_VALUE) + Number.MAX_VALUE !== -Number.MAX_VALUE + (Number.MAX_VALUE + Number.MAX_VALUE))
})();

(function tc_11_06_03__005() {
  assert((Infinity + Infinity === Infinity) && (-Infinity + -Infinity === -Infinity))
})();

(function tc_11_06_03__014() {
  assert(-Number.MAX_VALUE - -Number.MAX_VALUE === +0)
})();

(function tc_11_06_03__006() {
  assert(Infinity + 1 === Infinity)
})();

(function tc_11_06_03__025() {
  assert(("1" + 1) + 1 !== "1" + (1 + 1))
})();

(function tc_11_06_03__020() {
  assert(1e+308 - -1e+308 === +Infinity)
})();

(function tc_11_06_01__006() {
  var y = 1;
  assert(1 + y === 2)
})();

(function tc_11_06_01__012() {
  object = {
    valueOf: function () {
      return 1
    },
    toString: function () {
      return 0
    }
  }
  assert(1 + object === 2)
})();

(function tc_11_06_01__010() {
  assert(new Number(1) + new Number(1) === 2)
})();

(function tc_11_06_01__008() {
  var objectx = new Object();
  var objecty = new Object();
  objectx.prop = 1;
  objecty.prop = 1;
  assert(objectx.prop + objecty.prop === 2)
})();

(function tc_11_06_01__005() {
  var x = 1;
  assert(x + 1 === 2)
})();

(function tc_11_06_01__007() {
  assert(new Number(1) + 1 === 2)
})();

(function tc_11_06_01__002() {
  var a = 12;
  var b = "3";
  assert(a + b === "123")
})();

(function tc_11_06_01__017() {
  object = new Object()

  var b = 1

  assert(object + b === "[object Object]1")
})();

(function tc_11_06_01__015() {
  object1 = {
    valueOf: function () {
      return 1;
    },
    toString: function () {
      return 0;
    }
  }

  object2 = {
    valueOf: function () {
      return 1;
    },
    toString: function () {
      return 0;
    }
  }

  assert(object1 + object2 === 2)
})();

(function tc_11_06_01__009() {
  assert(1 + new Number(1) === 2)
})();

(function tc_11_06_01__018() {
  object = new String()

  var b = 1

  assert(object + b === "1")
})();

(function tc_11_06_01__003() {
  var a = "12";
  var b = 3;
  assert(a + b === "123")
})();

(function tc_11_06_01__011() {
  object = {
    valueOf: function () {
      return 1
    },
    toString: function () {
      return 0
    }
  }
  assert(object + 1 === 2)
})();

(function tc_11_06_01__016() {
  object = new Object()

  var str = new String()

  assert(object + str === "[object Object]")
})();

(function tc_11_06_01__001() {
  var a = "lirum ";
  var b = "ipsum";
  assert(a + b == "lirum ipsum")
})();

(function tc_11_06_01__013() {
  object = {
    valueOf: function () {
      return 1
    },
    toString: function () {
      return 0
    }
  }
  assert(object + "1" === "11")
})();

(function tc_11_06_01__014() {
  object = {
    valueOf: function () {
      return "1"
    },
    toString: function () {
      return 0
    }
  }
  assert("1" + object === "11")
})();

(function tc_11_06_01__004() {
  var a = 123;
  var b = 456;
  assert(a + b == 579)
})();

(function tc_11_09_04__020() {
  var x = "12.6asdg$7_sfk/sf/adf\.3rqaf\u0102", y = "12.6asdg$7_sfk/sf/adf\.3rqaf\u0102"
  assert(x === y)
})();

(function tc_11_09_04__007() {
  var x = false, y = 0
  assert(x !== y)
})();

(function tc_11_09_04__015() {
  var x = NaN, y = NaN
  assert(x !== y)
})();

(function tc_11_09_04__017() {
  var x = 123.01, y = 0.0123e+4
  assert(x !== y)
})();

(function tc_11_09_04__006() {
  var x = null, y = 0
  assert(x !== y)
})();

(function tc_11_09_04__001() {
  var x, y = null
  assert(x !== y)
})();

(function tc_11_09_04__019() {
  var x = -0, y = +0
  assert(x === y)
})();

(function tc_11_09_04__023() {
  var x = false, y = true
  assert(x !== y)
})();

(function tc_11_09_04__008() {
  var x = "0", y = 0
  assert(x !== y)
})();

(function tc_11_09_04__024() {
  var x = new String("abc")
  var y = x
  assert(x === y)
})();

(function tc_11_09_04__011() {
  var x, y
  assert(x === y)
})();

(function tc_11_09_04__005() {
  var x, y = new Function()
  assert(x !== y)
})();

(function tc_11_09_04__013() {
  var x = NaN, y = 0
  assert(x !== y)
})();

(function tc_11_09_04__016() {
  var x = 123.00, y = 0.0123e+4, eps = .00001
  assert(x <= y + eps && x >= y - eps)
})();

(function tc_11_09_04__025() {
  var x = new String("abc")
  var y = new String("abc")

  assert(x !== y)
})();

(function tc_11_09_04__009() {
  var x = "0", y = Object(0)
  assert(x !== y)
})();

(function tc_11_09_04__004() {
  var x, y = -37.2e-6
  assert(x !== y)
})();

(function tc_11_09_04__002() {
  var x, y = true
  assert(x !== y)
})();

(function tc_11_09_04__022() {
  var x = false, y = false
  assert(x === y)
})();

(function tc_11_09_04__014() {
  var x = 0.0, y = NaN
  assert(x !== y)
})();

(function tc_11_09_04__010() {
  var x = "abc", y = new String("abc")

  assert(x !== y)
})();

(function tc_11_09_04__021() {
  var x = true, y = true
  assert(x === y)
})();

(function tc_11_09_04__018() {
  var x = +0, y = -0
  assert(x === y)
})();

(function tc_11_09_04__012() {
  var x = y = null
  assert(x === y)
})();

(function tc_11_09_04__003() {
  var x, y = "undefined"
  assert(x !== y)
})();

(function tc_11_09_01__031() {
  var x = Object("abc")
  var y = Object("abc")
  b = x, c = y
  assert(!(c == b))
})();

(function tc_11_09_01__025() {
  var x = false, y = "-0"
  assert(x == y)
})();

(function tc_11_09_01__027() {
  var x = true, y = "-1"
  assert(!(x == y))
})();

(function tc_11_09_01__032() {
  var x = "a"
  var y = 2
  assert(!(x == y))
})();

(function tc_11_09_01__002() {
  var x = null, y = null
  assert(x == y)
})();

(function tc_11_09_01__005() {
  var x = 2.756, y = 2.756
  assert(x == y)
})();

(function tc_11_09_01__007() {
  var x = -0, y = +0
  assert(x == y)
})();

(function tc_11_09_01__006() {
  var x = +0, y = -0
  assert(x == y)
})();

(function tc_11_09_01__008() {
  var x = 2.8, y = 3.4
  assert(!(x == y))
})();

(function tc_11_09_01__033() {
  var x = "12.1e5"
  var y = 1210000
  assert(x == y)
})();

(function tc_11_09_01__036() {
  var x = 1e-324
  var y = false
  assert(x == y)
})();

(function tc_11_09_01__011() {
  var x = "abg", y = 'abh'
  assert(!(x == y))
})();

(function tc_11_09_01__014() {
  var x = false, y = false
  assert(x == y)
})();

(function tc_11_09_01__020() {
  var x = 0.123, y = "0.123e0"
  assert(x == y)
})();

(function tc_11_09_01__017() {
  var x = undefined, y = null
  assert(x == y)
})();

(function tc_11_09_01__004() {
  var x = 2, y = NaN
  assert(!(x == y))
})();

(function tc_11_09_01__024() {
  var x = 0.123, y = "1.23e-1"
  assert(x == y)
})();

(function tc_11_09_01__003() {
  var x = NaN, y = 1
  assert(!(x == y))
})();

(function tc_11_09_01__034() {
  var x = "1"
  var y = true
  assert(x == y)
})();

(function tc_11_09_01__021() {
  var x = 0.123, y = "0.123e+2"
  assert(!(x == y))
})();

(function tc_11_09_01__019() {
  var x = 0.123, y = "0.124"
  assert(!(x == y))
})();

(function tc_11_09_01__026() {
  var x = true, y = "+1"
  assert(x == y)
})();

(function tc_11_09_01__018() {
  var x = 0.123, y = "0.123"
  assert(x == y)
})();

(function tc_11_09_01__010() {
  var x = "abg", y = 'abgs'
  assert(x != y)
})();

(function tc_11_09_01__038() {
  var x = "0", y = Object(0)
  assert(x == y)
})();

(function tc_11_09_01__015() {
  var x = false, y = true
  assert(!(x == y))
})();

(function tc_11_09_01__009() {
  var x = "abg", y = 'abg'
  assert(x == y)
})();

(function tc_11_09_01__029() {
  var x = true, y = "123"
  assert(!(x == y))
})();

(function tc_11_09_01__016() {
  var x = null, y = undefined
  assert(x == y)
})();

(function tc_11_09_01__035() {
  var x = 0
  var y = false
  assert(x == y)
})();

(function tc_11_09_01__037() {
  var x = 0.1e-323
  var y = false
  assert(x == y)
})();

(function tc_11_09_01__013() {
  var x = true, y = true
  assert(x == y)
})();

(function tc_11_09_01__028() {
  var x = true, y = "true"
  assert(!(x == y))
})();

(function tc_11_09_01__030() {
  var x = Object("abc")
  b = x
  assert(x == b)
})();

(function tc_11_09_01__022() {
  var x = 0.123, y = "0.123a"
  assert(!(x == y))
})();

(function tc_11_09_01__023() {
  var x = 0.123, y = "b0.123"
  assert(!(x == y))
})();

(function tc_11_09_01__012() {
  var x = "abg", y = 'aBg'
  assert(!(x == y))
})();

(function tc_11_09_01__001() {
  var x, y
  assert(x == y)
})();

(function tc_11_09_02__035() {
  var x = 0
  var y = false
  assert(x == y)
})();

(function tc_11_09_02__033() {
  var x = "12.1e5"
  var y = 1210000
  assert(!(x != y))
})();

(function tc_11_09_02__019() {
  var x = 0.123, y = "0.124"
  assert(x != y)
})();

(function tc_11_09_02__018() {
  var x = 0.123, y = "0.123"
  assert(x == y)
})();

(function tc_11_09_02__005() {
  var x = 2.756, y = 2.756
  assert(x == y)
})();

(function tc_11_09_02__028() {
  var x = true, y = "true"
  assert(x != y)
})();

(function tc_11_09_02__024() {
  var x = 0.123, y = "1.23e-1"
  assert(x == y)
})();

(function tc_11_09_02__032() {
  var x = "a"
  var y = 2
  assert(x != y)
})();

(function tc_11_09_02__021() {
  var x = 0.123, y = "0.123e+2"
  assert(!(x == y))
})();

(function tc_11_09_02__002() {
  var x = null, y = null
  assert(x == y)
})();

(function tc_11_09_02__026() {
  var x = true, y = "+1"
  assert(x == y)
})();

(function tc_11_09_02__037() {
  var x = 1e-323
  var y = false
  assert(x != y)
})();

(function tc_11_09_02__012() {
  var x = "abg", y = 'aBg'
  assert(x != y)
})();

(function tc_11_09_02__016() {
  var x = null, y = undefined
  assert(x == y)
})();

(function tc_11_09_02__017() {
  var x = undefined, y = null
  assert(x == y)
})();

(function tc_11_09_02__031() {
  var x = Object("abc")
  var y = Object("abc")
  b = x, c = y
  assert(c != b)
})();

(function tc_11_09_02__023() {
  var x = 0.123, y = "b0.123"
  assert(x != y)
})();

(function tc_11_09_02__015() {
  var x = false, y = true
  assert(x != y)
})();

(function tc_11_09_02__034() {
  var x = "1"
  var y = true
  assert(x == y)
})();

(function tc_11_09_02__036() {
  var x = 1e-324
  var y = false
  assert(x == y)
})();

(function tc_11_09_02__022() {
  var x = 0.123, y = "0.123a"
  assert(x != y)
})();

(function tc_11_09_02__038() {
  var x = "0", y = Object(0)
  assert(x == y)
})();

(function tc_11_09_02__020() {
  var x = 0.123, y = "0.123e0"
  assert(x == y)
})();

(function tc_11_09_02__003() {
  var x = NaN, y = 1
  assert(x != y)
})();

(function tc_11_09_02__014() {
  var x = false, y = false
  assert(x == y)
})();

(function tc_11_09_02__011() {
  var x = "abg", y = 'abh'
  assert(x != y)
})();

(function tc_11_09_02__025() {
  var x = false, y = "-0"
  assert(x == y)
})();

(function tc_11_09_02__007() {
  var x = -0, y = +0
  assert(x == y)
})();

(function tc_11_09_02__030() {
  var x = Object("abc")
  b = x
  assert(x == b)
})();

(function tc_11_09_02__027() {
  var x = true, y = "-1"
  assert(x != y)
})();

(function tc_11_09_02__004() {
  var x = 2, y = NaN
  assert(x != y)
})();

(function tc_11_09_02__006() {
  var x = +0, y = -0
  assert(x == y)
})();

(function tc_11_09_02__013() {
  var x = true, y = true
  assert(x == y)
})();

(function tc_11_09_02__010() {
  var x = "abg", y = 'abgs'
  assert(x != y)
})();

(function tc_11_09_02__001() {
  var x, y
  assert(x == y)
})();

(function tc_11_09_02__029() {
  var x = true, y = "123"
  assert(x != y)
})();

(function tc_11_09_02__008() {
  var x = 2.8, y = 3.4
  assert(x != y)
})();

(function tc_11_09_02__009() {
  var x = "abg", y = 'abg'
  assert(x == y)
})();

(function tc_11_09_05__009() {
  var x = "0", y = Object(0)
  assert(x !== y)
})();

(function tc_11_09_05__020() {
  var x = "12.6asdg$7_sfk/sf/adf\.3rqaf\u0102", y = "12.6asdg$7_sfk/sf/adf\.3rqaf\u0102"
  assert(x === y)
})();

(function tc_11_09_05__002() {
  var x, y = true
  assert(x !== y)
})();

(function tc_11_09_05__023() {
  var x = false, y = true
  assert(x !== y)
})();

(function tc_11_09_05__007() {
  var x = false, y = 0
  assert(x !== y)
})();

(function tc_11_09_05__019() {
  var x = -0, y = +0
  assert(x === y)
})();

(function tc_11_09_05__004() {
  var x, y = -37.2e-6
  assert(x !== y)
})();

(function tc_11_09_05__013() {
  var x = NaN, y = 0
  assert(x !== y)
})();

(function tc_11_09_05__005() {
  var x, y = new Function()
  assert(x !== y)
})();

(function tc_11_09_05__003() {
  var x, y = "undefined"
  assert(x !== y)
})();

(function tc_11_09_05__018() {
  var x = +0, y = -0
  assert(x === y)
})();

(function tc_11_09_05__008() {
  var x = "0", y = 0
  assert(x !== y)
})();

(function tc_11_09_05__012() {
  var x = y = null
  assert(x === y)
})();

(function tc_11_09_05__006() {
  var x = null, y = 0
  assert(x !== y)
})();

(function tc_11_09_05__024() {
  var x = new String("abc")
  var y = x
  assert(x === y)
})();

(function tc_11_09_05__025() {
  var x = new String("abc")
  var y = new String("abc")
  assert(x !== y)
})();

(function tc_11_09_05__017() {
  var x = 123.01, y = 0.0123e+4
  assert(x !== y)
})();

(function tc_11_09_05__015() {
  var x = NaN, y = NaN
  assert(x !== y)
})();

(function tc_11_09_05__010() {
  var x = "abc", y = new String("abc")
  assert(x !== y)
})();

(function tc_11_09_05__022() {
  var x = false, y = false
  assert(x === y)
})();

(function tc_11_09_05__001() {
  var x, y = null
  assert(x !== y)
})();

(function tc_11_09_05__021() {
  var x = true, y = true
  assert(x === y)
})();

(function tc_11_09_05__011() {
  var x, y
  assert(x === y)
})();

(function tc_11_09_05__016() {
  var x = 123.00, y = 0.0123e+4, eps = .00001
  assert(x <= y + eps && x >= y - eps)
})();

(function tc_11_09_05__014() {
  var x = 0.0, y = NaN
  assert(x !== y)
})();

(function tc_11_11__007() {
  var a = "";
  var b = new Object();
  assert(!((a && b) !== a))
})();

(function tc_11_11__001() {
  var a = true;
  var b = false;
  assert(!((a && b) === true))
})();

(function tc_11_11__019() {
  var a = false;
  var b = new Object;
  assert((a || b) === b)
})();

(function tc_11_11__022() {
  var a = 123.456;
  var b = new Object;
  assert((a || b) === a)
})();

(function tc_11_11__017() {
  var a = false;
  var b = false;
  assert((a || b) === false)
})();

(function tc_11_11__010() {
  var a = 12345;
  var b = new Object();
  assert((a && b) === b)
})();

(function tc_11_11__005() {
  var a = true;
  var b = new Object();
  assert((a && b) === b)
})();

(function tc_11_11__027() {
  var a = new Object;
  var b = "who cares, what is this?";
  assert((a || b) === a)
})();

(function tc_11_11__013() {
  var a = new Object;
  var b = new Object;
  assert((a && b) === b)
})();

(function tc_11_11__002() {
  var a = true;
  var b = true;
  assert((a && b) === true)
})();

(function tc_11_11__009() {
  var a = NaN;
  var b = new Object ();
  assert ((!a && b) === b);
})();

(function tc_11_11__008() {
  var a = 0;
  var b = new Object();
  assert((a && b) === 0)
})();

(function tc_11_11__006() {
  var a = "not empty string";
  var b = new Object();
  assert(!((a && b) !== b))
})();

(function tc_11_11__014() {
  var a = true;
  var b = false;
  assert((a || b) === true)
})();

(function tc_11_11__004() {
  var a = false;
  var b = new Object();
  assert((a && b) === false)
})();

(function tc_11_11__024() {
  var a = "";
  var b = new Object;
  assert((a || b) === b)
})();

(function tc_11_11__018() {
  var a = true;
  var b = new Object;
  assert((a || b) === true)
})();

(function tc_11_11__026() {
  var a = undefined;
  var b = new String("123");
  assert((a || b) === b)
})();

(function tc_11_11__023() {
  var a = "non empty string";
  var b = new Object;
  assert((a || b) === a)
})();

(function tc_11_11__011() {
  var a = null;
  var b = new Object();
  assert((a && b) === a)
})();

(function tc_11_11__020() {
  var a1 = +0;
  var a2 = -0;
  var b = new Object;
  assert(((a1 || b) === b) && ((a2 || b) === b))
})();

(function tc_11_11__021() {
  var a = NaN;
  var b = new Object;
  assert((a || b) === b)
})();

(function tc_11_11__003() {
  var a = false;
  var b = false;
  assert(!((a && b) === true))
})();

(function tc_11_11__025() {
  var a = null;
  var b = Number(123.5e5);
  assert((a || b) === b)
})();

(function tc_11_11__012() {
  var a = undefined;
  var b = new Object();
  assert((a && b) === a)
})();

(function tc_11_11__016() {
  var a = false;
  var b = true;
  assert((a || b) === true)
})();

(function tc_11_11__015() {
  var a = true;
  var b = true;
  assert((a || b) === true)
})();

(function tc_11_12__004() {
  var a = 1;
  var b = 2;
  var c = 1;
  var d = 1;

  var e;

  e = (a < b) ? (c = 100) : (d = 10);

  assert(c == 100 && e == 100 && d == 1)
})();

(function tc_11_12__008() {
  var cond = 23;
  var a = [1, 2, 4];
  var cnt = 0;

  for (var i = (cond < 24) ? 0 in a : 2; i < 10; ++i)
  {
    ++cnt;
  }

  assert(cnt == 9)
})();

(function tc_11_12__005() {
  var a = 1;
  var b = 2;
  var c = 1;
  var d = 1;

  var e;

  e = (a > b) ? (c = 100) : (d = 10);

  assert(c == 1 && e == 10 && d == 10)
})();

(function tc_11_12__003() {
  var a = 1;
  var b = 2;
  var c = 3;

  var d;

  d = a > b ? 5 : b < c ? 10 : 15;

  assert(d == 10)
})();

(function tc_11_12__012() {
  var cond = 12;
  var res;

  res = (cond < 13)
          ?
          1
          :
          2;

  assert(res == 1)
})();

(function tc_11_12__001() {
  var a = 5;
  var b = 3;

  var c = (a > b) ? 12 : 14;

  assert(c == 12)
})();

(function tc_11_12__002() {
  var a = 1;
  var b = 2;
  var c = 3;

  var d;

  d = a < b ? b < c ? 5 : 10 : 15;

  assert(d == 5)
})();

(function tc_11_05_02__017() {
  assert(isNaN(null / undefined) === true)
})();

(function tc_11_05_02__036() {
  assert(isNaN(false / null) === true)
})();

(function tc_11_05_02__056() {
  assert(new String("2") / new Number(1) === 2)
})();

(function tc_11_05_02__045() {
  assert(isNaN(Number.NaN / 2) === true)
})();

(function tc_11_05_02__021() {
  assert("6" / 2 === 3)
})();

(function tc_11_05_02__028() {
  assert(true / "1" === 1)
})();

(function tc_11_05_02__009() {
  assert(false / true === 0)
})();

(function tc_11_05_02__080() {
  assert(Number.NEGATIVE_INFINITY / 2 === Number.NEGATIVE_INFINITY)
})();

(function tc_11_05_02__073() {
  assert(0 / Number.NEGATIVE_INFINITY === 0)
})();

(function tc_11_05_02__039() {
  assert(isNaN(Number.NaN / +0) === true)
})();

(function tc_11_05_02__031() {
  assert(isNaN(undefined / "2") === true)
})();

(function tc_11_05_02__058() {
  assert(new String("2") / new String("1") === 2)
})();

(function tc_11_05_02__065() {
  assert(new Boolean(true) / null === Number.POSITIVE_INFINITY)
})();

(function tc_11_05_02__005() {
  var a = 0;
  assert((a = 1) / a === 1)
})();

(function tc_11_05_02__040() {
  assert(isNaN(Number.NaN / -0) === true)
})();

(function tc_11_05_02__074() {
  assert(0 / Number.POSITIVE_INFINITY === 0)
})();

(function tc_11_05_02__008() {
  assert(true / true === 1)
})();

(function tc_11_05_02__088() {
  assert(Number.MIN_VALUE / 2 === 0)
})();

(function tc_11_05_02__051() {
  assert(false / new Boolean(true) === 0)
})();

(function tc_11_05_02__075() {
  assert(Number.POSITIVE_INFINITY / 0 === Number.POSITIVE_INFINITY)
})();

(function tc_11_05_02__024() {
  assert(1 / null === Number.POSITIVE_INFINITY)
})();

(function tc_11_05_02__087() {
  assert(Number.MAX_VALUE / (-0.5) === Number.NEGATIVE_INFINITY)
})();

(function tc_11_05_02__019() {
  assert(2 / true === 2)
})();

(function tc_11_05_02__067() {
  assert(isNaN(undefined / new Boolean(true)) === true)
})();

(function tc_11_05_02__035() {
  assert(isNaN(undefined / true) === true)
})();

(function tc_11_05_02__082() {
  assert(2 / Number.POSITIVE_INFINITY === 0)
})();

(function tc_11_05_02__002() {
  var a = 3;
  assert(6 / a === 2)
})();

(function tc_11_05_02__059() {
  assert(isNaN({} / {}) === true)
})();

(function tc_11_05_02__038() {
  assert(isNaN(Number.NaN / Number.NaN) === true)
})();

(function tc_11_05_02__022() {
  assert(isNaN("a" / 2) === true)
})();

(function tc_11_05_02__060() {
  assert(null / new Number(5) === 0)
})();

(function tc_11_05_02__057() {
  assert(new String("2") / 1 === 2)
})();

(function tc_11_05_02__006() {
  var a = 0;
  assert(a / (a = 1) === 0)
})();

(function tc_11_05_02__043() {
  assert(isNaN(Number.NaN / Number.MAX_VALUE) === true)
})();

(function tc_11_05_02__025() {
  assert(null / 1 === 0)
})();

(function tc_11_05_02__055() {
  assert(false / new String("2") === 0)
})();

(function tc_11_05_02__029() {
  assert("2" / true === 2)
})();

(function tc_11_05_02__026() {
  assert(isNaN(1 / undefined) === true)
})();

(function tc_11_05_02__054() {
  assert(new Boolean(false) / new String("2") === 0)
})();

(function tc_11_05_02__044() {
  assert(isNaN(Number.NaN / Number.MIN_VALUE) === true)
})();

(function tc_11_05_02__030() {
  assert(isNaN("2" / undefined) === true)
})();

(function tc_11_05_02__001() {
  assert(6 / 3 === 2)
})();

(function tc_11_05_02__037() {
  assert(null / true === 0)
})();

(function tc_11_05_02__081() {
  assert(2 / Number.NEGATIVE_INFINITY === -0)
})();

(function tc_11_05_02__032() {
  assert("2" / null === Number.POSITIVE_INFINITY)
})();

(function tc_11_05_02__047() {
  assert(6 / new Number(3) === 2)
})();

(function tc_11_05_02__048() {
  assert(new Number(6) / new Number(3) === 2)
})();

(function tc_11_05_02__089() {
  assert(2 / Number.MIN_VALUE === Number.POSITIVE_INFINITY)
})();

(function tc_11_05_02__012() {
  assert(isNaN("a" / "3") === true)
})();

(function tc_11_05_02__084() {
  assert(isNaN(0 / 0) === true)
})();

(function tc_11_05_02__086() {
  assert(Number.MAX_VALUE / Number.MAX_VALUE === 1)
})();

(function tc_11_05_02__085() {
  assert(Number.MAX_VALUE / 0.5 === Number.POSITIVE_INFINITY)
})();

(function tc_11_05_02__052() {
  assert(new Boolean(true) / false === Number.POSITIVE_INFINITY)
})();

(function tc_11_05_02__004() {
  var a = 6;
  var b = 3;
  assert(a / b === 2)
})();

(function tc_11_05_02__033() {
  assert(null / "2" === 0)
})();

(function tc_11_05_02__053() {
  assert(new String("2") / new Boolean(false) === Number.POSITIVE_INFINITY)
})();

(function tc_11_05_02__014() {
  assert(isNaN(null / null) === true)
})();

(function tc_11_05_02__042() {
  assert(isNaN(Number.NaN / Number.NEGATIVE_INFINITY) === true)
})();

(function tc_11_05_02__010() {
  assert(true / false === Number.POSITIVE_INFINITY)
})();

(function tc_11_05_02__069() {
  assert(isNaN(new String("5") / undefined) === true)
})();

(function tc_11_05_02__076() {
  assert(isNaN(Number.POSITIVE_INFINITY / Number.POSITIVE_INFINITY) === true)
})();

(function tc_11_05_02__023() {
  assert(isNaN(6 / "a") === true)
})();

(function tc_11_05_02__018() {
  assert(true / 1 === 1)
})();

(function tc_11_05_02__079() {
  assert(isNaN(Number.NEGATIVE_INFINITY / Number.NEGATIVE_INFINITY) === true)
})();

(function tc_11_05_02__049() {
  assert(new Number(2) / new Boolean(true) === 2)
})();

(function tc_11_05_02__062() {
  assert(new String("5") / null === Number.POSITIVE_INFINITY)
})();

(function tc_11_05_02__078() {
  assert(isNaN(Number.NEGATIVE_INFINITY / Number.POSITIVE_INFINITY) === true)
})();

(function tc_11_05_02__083() {
  assert(Number.POSITIVE_INFINITY / 2 === Number.POSITIVE_INFINITY)
})();

(function tc_11_05_02__034() {
  assert(isNaN(true / undefined) === true)
})();

(function tc_11_05_02__068() {
  assert(isNaN(undefined / new String("5")) === true)
})();

(function tc_11_05_02__013() {
  assert(isNaN("6" / "a") === true)
})();

(function tc_11_05_02__090() {
  assert((1 / 2) / 4 !== 1 / (2 / 4))
})();

(function tc_11_05_02__072() {
  assert(Number.NEGATIVE_INFINITY / 0 === Number.NEGATIVE_INFINITY)
})();

(function tc_11_05_02__016() {
  assert(isNaN(undefined / null) === true)
})();

(function tc_11_05_02__003() {
  var a = 6;
  assert(a / 3 === 2)
})();

(function tc_11_05_02__061() {
  assert(new Number(5) / null === Number.POSITIVE_INFINITY)
})();

(function tc_11_05_02__066() {
  assert(isNaN(new Boolean(true) / undefined) === true)
})();

(function tc_11_05_02__077() {
  assert(isNaN(Number.POSITIVE_INFINITY / Number.NEGATIVE_INFINITY) === true)
})();

(function tc_11_05_02__046() {
  assert(new Number(6) / 3 === 2)
})();

(function tc_11_05_02__027() {
  assert(isNaN(undefined / 1) === true)
})();

(function tc_11_05_02__015() {
  assert(isNaN(undefined / undefined) === true)
})();

(function tc_11_05_02__020() {
  assert(6 / "2" === 3)
})();

(function tc_11_05_02__063() {
  assert(null / new String("5") === 0)
})();

(function tc_11_05_02__041() {
  assert(isNaN(Number.NaN / Number.POSITIVE_INFINITY) === true)
})();

(function tc_11_05_02__011() {
  assert("6" / "3" === 2)
})();

(function tc_11_05_02__050() {
  assert(new Boolean(false) / new Boolean(true) === 0)
})();

(function tc_11_05_02__070() {
  assert(isNaN(new Number(5) / undefined) === true)
})();

(function tc_11_05_02__064() {
  assert(null / new Boolean(true) === 0)
})();

(function tc_11_05_02__071() {
  assert(isNaN(undefined / new Number(5)) === true)
})();

(function tc_11_05_02__007() {
  var a = 6;
  assert(a /
          3 === 2)
})();

(function tc_11_05_01__018() {
  assert(true * 1 === 1)
})();

(function tc_11_05_01__015() {
  assert(isNaN(undefined * undefined) === true)
})();

(function tc_11_05_01__085() {
  assert(2 * Number.MAX_VALUE === Number.POSITIVE_INFINITY)
})();

(function tc_11_05_01__048() {
  assert(new Number(1) * new Number(1) === 1)
})();

(function tc_11_05_01__049() {
  assert(new Number(1) * new Boolean(true) === 1)
})();

(function tc_11_05_01__050() {
  assert(new Boolean(true) * new Boolean(true) === 1)
})();

(function tc_11_05_01__010() {
  assert(false * false === 0)
})();

(function tc_11_05_01__078() {
  assert(Number.NEGATIVE_INFINITY * Number.POSITIVE_INFINITY === Number.NEGATIVE_INFINITY)
})();

(function tc_11_05_01__038() {
  assert(isNaN(Number.NaN * Number.NaN) === true)
})();

(function tc_11_05_01__064() {
  assert(null * new Boolean(true) === 0)
})();

(function tc_11_05_01__067() {
  assert(isNaN(undefined * new Boolean(true)) === true)
})();

(function tc_11_05_01__058() {
  assert(new String("2") * new String("1") === 2)
})();

(function tc_11_05_01__005() {
  var a = 0;
  assert((a = 1) * a === 1)
})();

(function tc_11_05_01__001() {
  assert(2 * 3 === 6)
})();

(function tc_11_05_01__087() {
  assert(Number.MAX_VALUE * (-1.5) === Number.NEGATIVE_INFINITY)
})();

(function tc_11_05_01__083() {
  assert(Number.POSITIVE_INFINITY * 1 === Number.POSITIVE_INFINITY)
})();

(function tc_11_05_01__069() {
  assert(isNaN(new String("1") * undefined) === true)
})();

(function tc_11_05_01__074() {
  assert(isNaN(0 * Number.POSITIVE_INFINITY) === true)
})();

(function tc_11_05_01__029() {
  assert("1" * true === 1)
})();

(function tc_11_05_01__034() {
  assert(isNaN(true * undefined) === true)
})();

(function tc_11_05_01__068() {
  assert(isNaN(undefined * new String("1")) === true)
})();

(function tc_11_05_01__061() {
  assert(new Number(2) * null === 0)
})();

(function tc_11_05_01__077() {
  assert(Number.POSITIVE_INFINITY * Number.NEGATIVE_INFINITY === Number.NEGATIVE_INFINITY)
})();

(function tc_11_05_01__080() {
  assert(Number.NEGATIVE_INFINITY * 1 === Number.NEGATIVE_INFINITY)
})();

(function tc_11_05_01__022() {
  assert(isNaN("a" * 1) === true)
})();

(function tc_11_05_01__019() {
  assert(1 * true === 1)
})();

(function tc_11_05_01__006() {
  var a = 0;
  assert(a * (a = 1) === 0)
})();

(function tc_11_05_01__054() {
  assert(new String("2") * new Boolean(true) === 2)
})();

(function tc_11_05_01__013() {
  assert(isNaN("1" * "a") === true)
})();

(function tc_11_05_01__021() {
  assert("2" * 3 === 6)
})();

(function tc_11_05_01__063() {
  assert(null * new String("2") === 0)
})();

(function tc_11_05_01__035() {
  assert(isNaN(undefined * true) === true)
})();

(function tc_11_05_01__056() {
  assert(new String("2") * new Number(1) === 2)
})();

(function tc_11_05_01__031() {
  assert(isNaN(undefined * "1") === true)
})();

(function tc_11_05_01__084() {
  assert(Number.MAX_VALUE * 2 === Number.POSITIVE_INFINITY)
})();

(function tc_11_05_01__025() {
  assert(null * 1 === 0)
})();

(function tc_11_05_01__042() {
  assert(isNaN(Number.NaN * Number.NEGATIVE_INFINITY) === true)
})();

(function tc_11_05_01__079() {
  assert(Number.NEGATIVE_INFINITY * Number.NEGATIVE_INFINITY === Number.POSITIVE_INFINITY)
})();

(function tc_11_05_01__040() {
  assert(isNaN(Number.NaN * -0) === true)
})();

(function tc_11_05_01__007() {
  var a = 2;
  assert(a *
          3 === 6)
})();

(function tc_11_05_01__070() {
  assert(isNaN(new Number(1) * undefined) === true)
})();

(function tc_11_05_01__055() {
  assert(new String("2") * true === 2)
})();

(function tc_11_05_01__041() {
  assert(isNaN(Number.NaN * Number.POSITIVE_INFINITY) === true)
})();

(function tc_11_05_01__003() {
  var a = 3;
  assert(a * 2 === 6)
})();

(function tc_11_05_01__014() {
  assert(null * null === 0)
})();

(function tc_11_05_01__073() {
  assert(isNaN(0 * Number.NEGATIVE_INFINITY) === true)
})();

(function tc_11_05_01__002() {
  var a = 3;
  assert(2 * a === 6)
})();

(function tc_11_05_01__004() {
  var a = 3;
  var b = 2;
  assert(a * b === 6)
})();

(function tc_11_05_01__046() {
  assert(1 * new Number(1) === 1)
})();

(function tc_11_05_01__026() {
  assert(isNaN(1 * undefined) === true)
})();

(function tc_11_05_01__008() {
  assert(true * true === 1)
})();

(function tc_11_05_01__060() {
  assert(null * new Number(2) === 0)
})();

(function tc_11_05_01__028() {
  assert(true * "1" === 1)
})();

(function tc_11_05_01__089() {
  assert(0.1 * Number.MIN_VALUE === +0)
})();

(function tc_11_05_01__062() {
  assert(new String("2") * null === 0)
})();

(function tc_11_05_01__053() {
  assert(new Boolean(true) * new String("2") === 2)
})();

(function tc_11_05_01__037() {
  assert(null * true === 0)
})();

(function tc_11_05_01__036() {
  assert(true * null === 0)
})();

(function tc_11_05_01__047() {
  assert(new Number(1) * 1 === 1)
})();

(function tc_11_05_01__033() {
  assert(null * "1" === 0)
})();

(function tc_11_05_01__009() {
  assert(true * false === 0)
})();

(function tc_11_05_01__032() {
  assert("1" * null === 0)
})();

(function tc_11_05_01__043() {
  assert(isNaN(Number.NaN * Number.MAX_VALUE) === true)
})();

(function tc_11_05_01__051() {
  assert(true * new Boolean(true) === 1)
})();

(function tc_11_05_01__017() {
  assert(isNaN(null * undefined) === true)
})();

(function tc_11_05_01__024() {
  assert(1 * null === 0)
})();

(function tc_11_05_01__012() {
  assert(isNaN("a" * "1") === true)
})();

(function tc_11_05_01__066() {
  assert(isNaN(new Boolean(true) * undefined) === true)
})();

(function tc_11_05_01__076() {
  assert(Number.POSITIVE_INFINITY * Number.POSITIVE_INFINITY === Number.POSITIVE_INFINITY)
})();

(function tc_11_05_01__081() {
  assert((-1) * Number.NEGATIVE_INFINITY === Number.POSITIVE_INFINITY)
})();

(function tc_11_05_01__086() {
  assert((-1.5) * Number.MAX_VALUE === Number.NEGATIVE_INFINITY)
})();

(function tc_11_05_01__090() {
  assert((Number.MAX_VALUE * 1.1) * 0.9 !== Number.MAX_VALUE * (1.1 * 0.9))
})();

(function tc_11_05_01__020() {
  assert(2 * "3" === 6)
})();

(function tc_11_05_01__088() {
  assert(Number.MIN_VALUE * (-0.5) === -0)
})();

(function tc_11_05_01__016() {
  assert(isNaN(undefined * null) === true)
})();

(function tc_11_05_01__075() {
  assert(isNaN(Number.POSITIVE_INFINITY * 0) === true)
})();

(function tc_11_05_01__082() {
  assert((-1) * Number.POSITIVE_INFINITY === Number.NEGATIVE_INFINITY)
})();

(function tc_11_05_01__059() {
  assert(isNaN({} * {}) === true)
})();

(function tc_11_05_01__023() {
  assert(isNaN(1 * "a") === true)
})();

(function tc_11_05_01__057() {
  assert(new String("2") * 1 === 2)
})();

(function tc_11_05_01__052() {
  assert(new Boolean(true) * true === 1)
})();

(function tc_11_05_01__072() {
  assert(isNaN(Number.NEGATIVE_INFINITY * 0) === true)
})();

(function tc_11_05_01__039() {
  assert(isNaN(Number.NaN * +0) === true)
})();

(function tc_11_05_01__045() {
  assert(isNaN(Number.NaN * 1) === true)
})();

(function tc_11_05_01__065() {
  assert(new Boolean(true) * null === 0)
})();

(function tc_11_05_01__011() {
  assert("2" * "3" === 6)
})();

(function tc_11_05_01__027() {
  assert(isNaN(undefined * 1) === true)
})();

(function tc_11_05_01__044() {
  assert(isNaN(Number.NaN * Number.MIN_VALUE) === true)
})();

(function tc_11_05_01__071() {
  assert(isNaN(undefined * new Number(1)))
})();

(function tc_11_05_01__030() {
  assert(isNaN("1" * undefined) === true)
})();

(function tc_11_05_03__005() {
  assert(100 % -3 > 0)
})();

(function tc_11_05_03__022() {
  var n = 100;
  var d = new Boolean(false);
  assert(isNaN(n % d))
})();

(function tc_11_05_03__013() {
  assert(isNaN(-Infinity % Infinity))
})();

(function tc_11_05_03__018() {
  var n = 105;
  var d = 10;
  assert(n % d === 5)
})();

(function tc_11_05_03__028() {
  assert(isNaN(1 % null))
})();

(function tc_11_05_03__002() {
  assert(isNaN(100 % NaN))
})();

(function tc_11_05_03__020() {
  var n = 550;
  var d = 5.5;
  assert(n % d === 0)
})();

(function tc_11_05_03__026() {
  assert(isNaN(undefined % 1))
})();

(function tc_11_05_03__007() {
  assert(isNaN(Infinity % 3))
})();

(function tc_11_05_03__027() {
  assert(null % 1 === +0)
})();

(function tc_11_05_03__030() {
  var n = new Boolean(true);
  var d = new String("");
  assert(isNaN(n % d))
})();

(function tc_11_05_03__015() {
  assert(+0 % 5 === +0)
})();

(function tc_11_05_03__012() {
  assert(0 % 5 === 0)
})();

(function tc_11_05_03__023() {
  var n = "100";
  var d = "";
  assert(isNaN(n % d))
})();

(function tc_11_05_03__003() {
  assert(isNaN(NaN % NaN))
})();

(function tc_11_05_03__010() {
  assert(isNaN(Infinity % 0))
})();

(function tc_11_05_03__024() {
  var n = {
    valueOf: function () {
      return 109.5;
    }
  }
  var d = {
    valueOf: function () {
      return 5.5;
    }
  }
  assert(n % d === 5)
})();

(function tc_11_05_03__029() {
  var n = new String("100");
  var d = new String("10");
  assert(n % d === 0)
})();

(function tc_11_05_03__019() {
  var n = 109.5;
  var d = 5.5;
  assert(n % d === 5)
})();

(function tc_11_05_03__008() {
  assert(isNaN(-Infinity % 3))
})();

(function tc_11_05_03__025() {
  var n = {
    valueOf: function () {
      return -Infinity;
    }
  }
  var d = {
    valueOf: function () {
      return 0;
    }
  }
  assert(isNaN(n % d))
})();

(function tc_11_05_03__021() {
  var n = 100;
  var d = new Boolean(true);
  assert(n % d === 0)
})();

(function tc_11_05_03__006() {
  assert(-100 % -3 < 0)
})();

(function tc_11_05_03__016() {
  var n = 100;
  var d = 10;
  assert(n % d === 0)
})();

(function tc_11_05_03__009() {
  assert(isNaN(5 % 0))
})();

(function tc_11_05_03__001() {
  assert(isNaN(NaN % 1))
})();

(function tc_11_05_03__017() {
  var n = 100.5;
  var d = 10;
  assert(n % d === 0.5)
})();

(function tc_11_05_03__011() {
  assert(5 % Infinity === 5)
})();

(function tc_11_05_03__004() {
  assert(-100 % 3 < 0)
})();

(function tc_11_05_03__014() {
  assert(-0 % 5 === -0)
})();

(function tc_11_03_02__006() {
  var a = true;
  var b = false;
  assert((a-- === 1) && (b-- === +0) && (a === 0) && (b === -1));
})();

(function tc_11_03_02__010() {
  var a = "1";
  assert((a-- === 1) && (a === 0));
})();

(function tc_11_03_02__015() {
  var a = "Infinity";
  assert((a-- === Infinity) && (a === Infinity));
})();

(function tc_11_03_02__013() {
  var a = "";
  assert((a-- === 0) && (a === -1));
})();

(function tc_11_03_02__016() {
  var a = "0xa";
  assert((a-- === 0xa) && (a === 9));
})();

(function tc_11_03_02__009() {
  var a = undefined;
  assert(isNaN(a--) && isNaN(a));
})();

(function tc_11_03_02__012() {
  var a = "1e3";
  assert((a-- === 1e3) && (a === 999));
})();

(function tc_11_03_02__011() {
  var a = "blah";
  assert(isNaN(a--) && isNaN(a));
})();

(function tc_11_03_02__014() {
  var a = "  ";

  assert((a-- === 0) && (a === -1));

})();

(function tc_11_03_02__008() {
  var a = null;
  assert((a-- == +0) && (a === -1));
})();

(function tc_11_03_02__005() {
  var a = 1;
  assert((a-- === 1) && (a === 0));
})();

(function tc_11_03_02__007() {
  var a = {
    valueOf: function () {
      return 1;
    }
  }

  assert((a-- === 1) && (a === 0));

})();

(function tc_11_03_01__007() {
  var a = {
    valueOf: function () {
      return 1;
    }
  }

  assert((a++ === 1) && (a === 2));
})();

(function tc_11_03_01__016() {
  var a = "0xa";
  assert((a++ === 0xa) && (a === 0xb));
})();

(function tc_11_03_01__014() {
  var a = "  ";
  assert((a++ === 0) && (a === 1));
})();

(function tc_11_03_01__015() {
  var a = "Infinity";

  assert((a++ === Infinity) && (a === Infinity));
})();

(function tc_11_03_01__009() {
  var a = undefined;

  assert(isNaN(a++) && isNaN(a));
})();

(function tc_11_03_01__005() {
  var a = 1;

  assert((a++ === 1) && (a === 2));
})();

(function tc_11_03_01__012() {
  var a = "1e3";

  assert((a++ === 1e3) && (a === 1001));

})();

(function tc_11_03_01__008() {
  var a = null;

  assert((a++ == +0) && (a === 1));
})();

(function tc_11_03_01__006() {
  var a = true;
  var b = false;

  assert((a++ === 1) && (b++ === +0) && (a === 2) && (b === 1));
})();

(function tc_11_03_01__011() {
  var a = "blah";
  assert ( isNaN(a++) && isNaN(a) );
})();

(function tc_11_03_01__013() {
  var a = "";
  assert((a++ === 0) && (a === 1));
})();

(function tc_11_03_01__010() {
  var a = "1";

  assert((a++ === 1) && (a === 2));
})();

(function tc_11_08_07__008() {
  var a = {name: 'Masha', 'age': 10}
  var c = 'Masha' in a
  assert(!c)
})();

(function tc_11_08_07__005() {
  var a = {name: 'Masha', 'age': 10}
  var c = 'name' in a
  assert(c)
})();

(function tc_11_08_07__010() {
  var a = new String('example');
  assert('toString' in a);
})();

(function tc_11_08_07__007() {
  var a = {name: 'Masha', 'age': 10}
  var c = "age" in a
  assert(c)
})();

(function tc_11_08_07__009() {
  var a = {name: 'Masha', 'age': 10}
  var c = "toString" in a
  assert(c)
})();

(function tc_11_08_07__014() {
  var c = 'PI' in Math
  assert(c)
})();

(function tc_11_08_07__011() {
  var a = new String('example')
  var c = 'length' in a
  assert(c)
})();

(function tc_11_08_07__013() {
  var a = new String('example')
  var c = 'toString' in a
  assert(c)
})();

(function tc_11_08_07__001() {
  var a = [1, 2, 3, 4, 5, 6]
  var c = 0 in a
  assert(c)
})();

(function tc_11_08_07__004() {
  var a = [1, 2, 3, 4, 5, 6]
  var c = '0' in a
  assert(c)
})();

(function tc_11_08_07__003() {
  var a = [1, 2, 3, 4, 5, 6]
  var c = 6 in a
  assert(!c)
})();

(function tc_11_08_07__002() {
  var a = [1, 2, 3, 4, 5, 6]
  var c = 5 in a
  assert(c)
})();

(function tc_11_08_07__012() {
  var a = new String('example');
  assert(!('toString.' in a));
})();

(function tc_11_08_01__006() {
  var a = false, b = true
  var c = a < b
  assert(c)
})();

(function tc_11_08_01__003() {
  var a = "abc", b = "abd"
  var c = a < b
  assert(c)
})();

(function tc_11_08_01__002() {
  var a = 7, b = 6
  var c = a < b
  assert(!c)
})();

(function tc_11_08_01__001() {
  var a = 5, b = 7
  var c = a < b
  assert(c)
})();

(function tc_11_08_01__005() {
  var a = null, b = undefined
  var c = a < b
  assert(!c)
})();

(function tc_11_08_01__004() {
  var a = "abd", b = "abc"
  var c = a < b
  assert(!c)
})();

(function tc_11_08_04__006() {
  var a = false, b = true
  var c = a >= b
  assert(!c)
})();

(function tc_11_08_04__005() {
  var a = null, b = undefined
  var c = a >= b
  assert(!c)
})();

(function tc_11_08_04__003() {
  var a = "zbda", b = "zbd"
  var c = a >= b
  assert(c)
})();

(function tc_11_08_04__001() {
  var a = 700000000000000000000, b = 500000000000000000000
  var c = a >= b
  assert(c)
})();

(function tc_11_08_04__008() {
  var a = 1.2, b = '1.2'
  var c = a >= b
  assert(c)
})();

(function tc_11_08_04__002() {
  var a = 6.233, b = 6.234
  var c = a >= b
  assert(!c)
})();

(function tc_11_08_04__009() {
  var a = 6.233, b = 6.233
  var c = a >= b
  assert(c)
})();

(function tc_11_08_04__004() {
  var a = "aaaaa1", b = "aaaaaz"
  var c = a >= b
  assert(!c)
})();

(function tc_11_08_04__007() {
  var a = false, b = false
  var c = a >= b
  assert(c)
})();

(function tc_11_08_03__010() {
  var a = 2, b = function () {
  }
  var c = a <= b
  assert(!c)
})();

(function tc_11_08_03__001() {
  var a = 5, b = 7
  var c = a <= b
  assert(c)
})();

(function tc_11_08_03__008() {
  var a = "abd", b = "abd"
  var c = a <= b
  assert(c)
})();

(function tc_11_08_03__005() {
  var a = null, b = undefined
  var c = a <= b
  assert(!c)
})();

(function tc_11_08_03__007() {
  var a = 7, b = 7
  var c = a <= b
  assert(c)
})();

(function tc_11_08_03__002() {
  var a = 7, b = 6
  var c = a <= b
  assert(!c)
})();

(function tc_11_08_03__011() {
  var a = 2, b = 'sdafg'
  var c = a <= b
  assert(!c)
})();

(function tc_11_08_03__009() {
  var a = true, b = true
  var c = a <= b
  assert(c)
})();

(function tc_11_08_03__003() {
  var a = "abc", b = "abd"
  var c = a <= b
  assert(c)
})();

(function tc_11_08_03__004() {
  var a = "abd", b = "abc"
  var c = a <= b
  assert(!c)
})();

(function tc_11_08_03__006() {
  var a = false, b = true
  var c = a <= b
  assert(c)
})();

(function tc_11_08_06__007() {
  var a = new Object()
  var c = a instanceof Object
  assert(c)
})();

(function tc_11_08_06__005() {
  var a = new String('abcd')
  var c = a instanceof String
  assert(c)
})();

(function tc_11_08_06__003() {
  var a = new Number(2)
  var c = a instanceof Number
  assert(c)
})();

(function tc_11_08_06__002() {
  var a = Number(2)
  var c = a instanceof Number
  assert(!c)
})();

(function tc_11_08_06__001() {
  var a = 2
  var c = a instanceof Number
  assert(!c)
})();

(function tc_11_08_06__008() {
  var a = null;
  var c = a instanceof Object;
  assert (!c);
})();

(function tc_11_08_06__004() {
  var a = 'abcd'
  var c = a instanceof String
  assert(!c)
})();

(function tc_11_08_06__006() {
  var a = function () {
  }
  var b = new a()
  var c = b instanceof a
  assert(c)
})();

(function tc_11_08_02__005() {
  var a = null, b = undefined
  var c = a > b
  assert(!c)
})();

(function tc_11_08_02__004() {
  var a = "aaaaa1", b = "aaaaaz"
  var c = a > b
  assert(!c)
})();

(function tc_11_08_02__003() {
  var a = "zbda", b = "zbd"
  var c = a > b
  assert(c)
})();

(function tc_11_08_02__006() {
  var a = false, b = true
  var c = a > b
  assert(!c)
})();

(function tc_11_08_02__001() {
  var a = 700000000000000000000, b = 500000000000000000000
  var c = a > b
  assert(c)
})();

(function tc_11_08_02__002() {
  var a = 6.233, b = 6.234
  var c = a > b
  assert(!c)
})();
