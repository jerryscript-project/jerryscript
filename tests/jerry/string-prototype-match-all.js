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

assert(String.prototype.matchAll.length === 1);
var desc = Object.getOwnPropertyDescriptor(String.prototype.matchAll, "length");
assert(!desc.enumerable);
assert(!desc.writable);
assert(desc.configurable);

assert(String.prototype.matchAll.name === "matchAll");
var desc = Object.getOwnPropertyDescriptor(String.prototype.matchAll, "name");
assert(!desc.enumerable);
assert(!desc.writable);
assert(desc.configurable);

// test basic functionality
var re = /[0-9]+/g;
var str = '2016-01-02|2019-03-07';
var result = str.matchAll(re);
var result_array = Array.from(result, x => x[0]);
assert(result_array.toString() === "2016,01,02,2019,03,07");

var counter = 0;
var obj = {};
RegExp.prototype[Symbol.matchAll] = function() {
  counter++;
  return obj;
};

assert('a'.matchAll(null) === obj);
assert(counter === 1);

assert(''.matchAll(undefined) === obj);
assert(counter === 2) ;

var obj = {};
var retval = {};
var counter = 0;
var thisVal, args;

obj[Symbol.matchAll] = function () {
    counter++;
  thisVal = this;
  args = arguments;
  return retval;
};

var str = ''

assert(str.matchAll(obj) === retval);
assert(counter === 1);
assert(thisVal === obj);
assert(args !== undefined);
assert(args.length === 1);
assert(args[0] === str);

// throw error when flag is not global
try {
  "foo".matchAll(/a/);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  "foo".matchAll(/a/i);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  "foo".matchAll(/a/m);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  "foo".matchAll(/a/u);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  "foo".matchAll(/a/u);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  "foo".matchAll(/a/y);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

var regexp = /a/;
Object.defineProperty(regexp, 'flags', {
  value: 'muyi'
});

try {
  "foo".matchAll(regexp);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// test when flags value is undefined
var regexp = /a/g;
Object.defineProperty(regexp, 'flags', { value: undefined });

try {
  "foo".matchAll(regexp)
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// test when match Symbol throws error
var regexp = /[A-Z]/g;
Object.defineProperty (regexp, Symbol.match, { get () { throw 42; }});

try {
  "foo".matchAll(regexp);
  assert(false);
} catch (e) {
  assert(e === 42);
}

// test when flags throws error
var regexp = /a/g;
Object.defineProperty(regexp, 'flags', { get () { throw 42; }});

try {
  "foo".matchAll(regexp);
  assert(false);
} catch (e) {
  assert(e === 42);
}

// test when flags value is symbol
var regexp = /a/g;
var sym = Symbol("foo")
Object.defineProperty(regexp, 'flags', { value: sym });

try {
  "foo".matchAll(regexp);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// test when matchAll Symbol throws error
var regexp = /[A-Z]/g;
Object.defineProperty (regexp, Symbol.matchAll, { get () { throw 42; }});

try {
  "foo".matchAll(regexp);
  assert(false);
} catch (e) {
  assert(e === 42);
}
