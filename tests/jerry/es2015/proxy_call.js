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

// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var target = function () {};
var handler = { apply (target) {
  throw 42;
}};

var proxy = new Proxy (target, handler);

try {
  // opfunc_call
  proxy (5)
  assert (false);
} catch (e) {
  assert (e == 42);
}

try {
  var revocable = Proxy.revokable (function () {}, {});
  proxy = new Proxy(revocable.proxy, {})
  revocable.revoke();
  proxy (5)
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

function sum (a, b) {
  return a + b;
}

var handler = {
  apply: function (target, thisArg, argumentsList) {
    return target (argumentsList[0], argumentsList[1]) * 10;
  }
};

var proxy1 = new Proxy(sum, handler);

assert (sum (1, 2) === 3);
assert (proxy1 (1, 2) === 30);

// Non Callable tests
var proxy = new Proxy ({},{});
try {
  proxy()
  assert (false)
} catch (e) {
  assert (e instanceof TypeError)
}

var proxy2 = new Proxy(proxy, {});
try {
  proxy2()
  assert (false)
} catch (e) {
  assert (e instanceof TypeError)
}

// No arguments
var called = false;
var target = function () {
  called = true;
}
var proxy = new Proxy (target, {});
assert (!called);
proxy();
assert (called);

called = false;
var proxy2 = new Proxy (proxy, {});
assert (!called);
proxy2();
assert (called);

//1 Argument
var called = false;
var target = function (a) {
  called = true;
  assert ('1' === a);
}
var proxy = new Proxy (target, {});
assert (!called);
proxy ('1');
assert (called);

// 2 Arguments
var called = false;
var target = function (a, b) {
  called = true;
  assert ('1' === a);
  assert ('2' === b);
}
var proxy = new Proxy (target, {});
assert (!called);
proxy ('1', '2');
assert (called);

// Changed receiver
var apply_receiver = {receiver:true};
var seen_receiver = undefined;
var target = function () {
  seen_receiver = this;
}
var proxy = new Proxy (target, {});
assert (undefined === seen_receiver);
Reflect.apply (proxy, apply_receiver, [1,2,3,4]);
assert (apply_receiver === seen_receiver);

// Trap
var called_target = false;
var called_handler = false;
var target = function (a, b) {
  called_target = true;
  assert (1 === a);
  assert (2 === b);
}
var handler = {
  apply: function (target, this_arg, args) {
    target.apply (this_arg, args);
    called_handler = true;
  }
}
var proxy = new Proxy (target, handler);
assert (!called_target);
assert (!called_handler);
Reflect.apply (proxy, {rec:1}, [1,2]);
assert (called_target);
assert (called_handler);

// Trap array arg
var called_target = false;
var called_handler = false;
var target = function (a, b) {
  called_target = true;
  var arg = [1, 2];
  assert (arg[0] === a[0]);
  assert (arg[1] === a[1]);
  assert (3 === b);
}
var handler = {
  apply: function (target, this_arg, args) {
    target.apply (this_arg, args);
    called_handler = true;
  }
}
var proxy = new Proxy (target, handler);
assert (!called_target);
assert (!called_handler);
proxy ([1,2], 3);
assert (called_target);
assert (called_handler);

// Trap object arg
var called_target = false;
var called_handler = false;
var target = function (o) {
  called_target = true;
  var obj = {a: 1, b: 2}
  assert (obj.a === o.a);
  assert (obj.b === o.b)
}
var handler = {
  apply: function (target, this_arg, args) {
    target.apply (this_arg, args);
    called_handler = true;
  }
}
var proxy = new Proxy (target, handler);
assert (!called_target);
assert (!called_handler);
proxy ({a: 1, b: 2});
assert (called_target);
assert (called_handler);

// Trap generator arg
function* gen () {
  yield 1;
  yield 2;
  yield 3;
}
var called_target = false;
var called_handler = false;
var target = function (g) {
  called_target = true;
  var arr = [1, 2, 3];
  var arr2 = [...g];
  assert (arr[0] === arr2[0]);
  assert (arr[1] === arr2[1]);
  assert (arr[2] === arr2[2]);
}
var handler = {
  apply: function (target, this_arg, args) {
    target.apply (this_arg, args);
    called_handler = true;
  }
}
var proxy = new Proxy (target, handler);
assert (!called_target);
assert (!called_handler);
proxy (gen());
assert (called_target);
assert (called_handler);

// Noncallable Trap
var called_target = false;
var target = function () {
  called_target = true;
};
var handler = {
  apply: 'non callable trap'
};

var proxy = new Proxy(target, handler);
try {
  proxy ();
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

assert (!called_target);

// Null trap
var _args;
var target = function (a, b) {
  _args = [a, b];
  return a + b;
};
var handler = {
  apply: null
};

var proxy = new Proxy (target, handler);
var result = proxy (1, 2);

assert (result === 3);
assert (_args.length === 2);
assert (_args[0] === 1);
assert (_args[1] === 2);

var values = [NaN, 1.5, 100, /RegExp/, "string", {}, [], Symbol(),
                new Map(), new Set(), new WeakMap(), new WeakSet()];
values.forEach(target => {
  target = Object (target);
  var proxy = new Proxy(target, { apply() { assert (false) } });
  try {
    proxy();
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  try {
    ({ proxy }).proxy();
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  try {
    Reflect.apply(proxy, null, []);
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  try {
    Reflect.apply(proxy, { proxy }, []);
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  try {
    Reflect.apply(proxy, { proxy }, []);
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  try {
    Function.prototype.call.apply (proxy, [null]);
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  try {
    Function.prototype.apply.apply (proxy, [null, []]);
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  var proxy_to_proxy = new Proxy (proxy, { apply() { assert (false); } });

  try {
    proxy_to_proxy ();
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  try {
    ({ proxy_to_proxy }).proxy_to_proxy();
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  try {
    Reflect.apply (proxy_to_proxy, null, []);
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  try {
    Reflect.apply (proxy_to_proxy, { proxy }, []);
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  try {
    Function.prototype.call.apply (proxy_to_proxy, [null]);
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  try {
    Function.prototype.apply.apply (proxy_to_proxy, [null, []]);
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }
});
