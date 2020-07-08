/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// test basic functionality
var order = [];
var reason = {};
var reject = Promise.reject(reason);
reject.then = function() {
  order.push(1);
  return Promise.prototype.then.apply(this, arguments);
};

var value = {};
var resolve = Promise.resolve(value);
resolve.then = function() {
  order.push(4);
  return Promise.prototype.then.apply(this, arguments);
};

reject.catch(function(e) {
	order.push(2);
	throw e;
}).finally(function() {
	order.push(3);
	return resolve;
}).catch(function(e) {
	order.push(5);
});

function __checkAsync() {
  assert(order.length === 5);
  for (var i = 0; i < order.length; i++)
  {
    assert(i + 1 === order[i]);
  }
}

// check length property
var desc = Object.getOwnPropertyDescriptor(Promise.prototype.finally, "length");
assert(desc.value === 1);
assert(desc.enumerable === false);
assert(desc.configurable === true);
assert(desc.writable === false);

// invokes then with function
var target = new Promise(function() {});
var returnValue = {};
var callCount = 0;
var thisValue = null;
var argCount = null;
var firstArg = null;
var secondArg = null;

target.then = function(a, b) {
  callCount += 1;

  thisValue = this;
  argCount = arguments.length;
  firstArg = a;
  secondArg = b;

  return returnValue;
};

var originalFinallyHandler = function() {};
var result = Promise.prototype.finally.call(target, originalFinallyHandler, 2, 3);

(callCount === 1);
assert(thisValue === target);
assert(argCount === 2);
assert(typeof firstArg === 'function');
assert(firstArg.length === 1);
assert(typeof secondArg === 'function');
assert(secondArg.length === 1);
assert(result === returnValue);

// invokes then with non-function
result = Promise.prototype.finally.call(target, 1, 2, 3);

assert(callCount === 2);
assert(thisValue === target);
assert(argCount === 2);
assert(firstArg === 1);
assert(secondArg === 1);
assert(result == returnValue);

// thes when 'then' is not callable
var thrower = function() {
  throw 42;
};

var symbol = Symbol();
var p = new Promise(function() {});

p.then = undefined;
try {
  Promise.prototype.finally.call(p, thrower);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

p.then = null;
try {
  Promise.prototype.finally.call(p, thrower);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

p.then = 1;
try {
  Promise.prototype.finally.call(p, thrower);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

p.then = symbol;
try {
  Promise.prototype.finally.call(p, thrower);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

p.then = {};
try {
  Promise.prototype.finally.call(p, thrower);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}
