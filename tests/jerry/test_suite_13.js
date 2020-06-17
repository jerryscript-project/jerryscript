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

(function tc_13__012() {
  function foo(a, b, c, d) {
    var deleted = true;
    for (i = 0; i < arguments.length; i++)
    {
      delete arguments[i];
      deleted = deleted && (typeof (arguments[i]) === "undefined");
    }
    return deleted;
  }

  assert(foo("A", "F", 1, true) === true);
})();

(function tc_13__005() {
  var foo = 1;

  assert (foo === 1);

  function foo()
  {
      return 1;
  }
})();

(function tc_13__013() {
  function foo(arguments) {
    return arguments;
  }

  assert(foo(1) === 1);
})();

(function tc_13__010() {
  function foo() {
    return 1;
  }
  var object = new Object;
  object.fun = foo;

  assert(object.fun() === 1);
})();

(function tc_13__001() {
  var b = 1;
  for (var i = 1; i < 10; ++i)
  {
    b *= i;
  }
  var c = b;

  assert(c == 362880);
})();

(function tc_13__007() {
  function foo(arg) {
    return ++arg;
  }

  assert(foo(1) === 2);
})();

(function tc_13__008() {
  function foo(params) {
    return arguments.length;
  }

  assert(foo(1, 'e', true, 5) === 4);
})();

(function tc_13__003() {
  assert(function (param1, param2) {
    return 1;
  }(true, "blah") === 1);
})();

(function tc_13__011() {
  function foo(param1) {
    return delete arguments;
  }

  assert(!foo("param"));
})();

(function tc_13__002() {
  function foo() {
    return 1;
  }

  assert(foo() === 1);
})();

(function tc_13__009() {
  var check = typeof (foo) === "function";

  var foo = 1;

  check = check && (foo === 1);

  function foo() {
    return 1;
  }

  assert(check);
})();

(function tc_13__006() {
  function foo() {
  }

  assert(foo() === undefined);
})();

(function tc_13__004() {
  function foo() {
  }

  assert(typeof foo === "function");
})();

(function tc_13_01__001() {
  function arguments (param) {
    return true;
  }
  assert (arguments ());
})();

(function tc_13_02__001() {
  var foo = function () {
    this.caller = 123;
  };
  var f = new foo();
  assert(f.caller === 123);
})();

(function tc_13_02__007() {
  var obj = new function foo()
  {
    this.prop = 1;
  };

  assert(obj.prop === 1);
})();

(function tc_13_02__003() {
  function foo(arg) {
    arg.prop = 3;
  }
  var obj = new Object();
  foo(obj);

  assert(obj.prop === 3);
})();

(function tc_13_02__006() {
  function foo() {
    return;
  }

  assert(foo() === undefined);
})();

(function tc_13_02__004() {
  function foo(arg) {
    arg += 3;
  }

  var num = 1;
  foo(num);

  assert(num === 1);
})();

(function tc_13_02__005() {
  function foo() {
    return null;
  }

  assert(foo() === null);
})();

(function tc_13_02__008() {
  function func() {
  }

  assert(Function.prototype.isPrototypeOf(func));
})();
