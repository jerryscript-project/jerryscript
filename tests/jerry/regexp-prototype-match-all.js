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

assert(RegExp.prototype[Symbol.matchAll].length === 1);
var desc = Object.getOwnPropertyDescriptor(RegExp.prototype[Symbol.matchAll], "length");
assert(!desc.enumerable);
assert(!desc.writable);
assert(desc.configurable);

assert(RegExp.prototype[Symbol.matchAll].name === "[Symbol.matchAll]");
var desc = Object.getOwnPropertyDescriptor(RegExp.prototype[Symbol.matchAll], "name");
assert(!desc.enumerable);
assert(!desc.writable);
assert(desc.configurable);

// test basic functionality
var re = /[0-9]+/g;
var str = '2016-01-02';
var result = re[Symbol.matchAll](str);
assert(Array.from(result, x => x[0]).toString() === "2016,01,02");

class MyRegExp extends RegExp {
  [Symbol.matchAll](str) {
    const result = RegExp.prototype[Symbol.matchAll].call(this, str);
    if (!result) {
      return null;
    }
    return Array.from(result);
  }
}
  
var regexp = new MyRegExp('-[0-9]+', 'g');
var result = re[Symbol.matchAll]("2016-01-02|2019-03-07");
assert(Array.from(result, x => x[0]).toString() === "2016,01,02,2019,03,07");

var counter = 0;
var callArgs;
var regexp = /\d/u;
regexp.constructor = {
  [Symbol.species]: function(){
    counter++;
    callArgs = arguments;
    return /\w/g;
  }
};

var str = 'a*b';
var result = regexp[Symbol.matchAll](str);

assert(counter === 1);
assert(callArgs.length === 2);
assert(callArgs[0] === regexp);
assert(callArgs[1] === 'u');
assert(Array.from(result, x => x[0]).toString() === "a");

// test when flags throws error
var regexp = /a/g;
Object.defineProperty(regexp, 'flags', { get () { throw 42; }});

try {
  regexp[Symbol.matchAll]("foo");
  assert(false);
} catch (e) {
  assert(e === 42);
}

// test when flags value is symbol
var regexp = /a/g;
var sym = Symbol("foo")
Object.defineProperty(regexp, 'flags', { value: sym });

try {
  regexp[Symbol.matchAll]("foo");
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// test when match Symbol throws error
var regexp = /[A-Z]/g;
Object.defineProperty (regexp, Symbol.match, { get () { throw 42; }});

try {
  regexp[Symbol.matchAll]("foo");
  assert(false);
} catch (e) {
  assert(e === 42);
}

// test when can't create RegExp
var obj = {
  toString() {
     throw 42;
   }
};

try {
  RegExp.prototype[Symbol.matchAll].call(obj, ''); 
  assert(false);
} catch (e) {
  assert(e === 42);
}

// test when constructor throws error
var regexp = /./;
Object.defineProperty(regexp, 'constructor', {
  get(){
    throw 42;
  }
});

try {
  regexp[Symbol.matchAll]("foo");
  assert(false);
} catch (e) {
  assert(e === 42);
}

// test when global flag throws error
var regexp = /[A-Z]/;
Object.defineProperty(regexp, 'global', { get() { throw 42; }});

try {
  regexp[Symbol.matchAll]('');
  assert(false);
} catch (e) {
  assert(e === 42);
}

// test when lastIndex throws error
var regexp = /[A-Z]/;
regexp.lastIndex = {
  valueOf() {
    throw 42;
  }
};

try {
  regexp[Symbol.matchAll]("foo");
  assert(false);
} catch (e) {
  assert(e === 42);
}

// test when argument is not an object
try {
  RegExp.prototype[Symbol.matchAll].call(null, '');
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  RegExp.prototype[Symbol.matchAll].call(true, '');
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  RegExp.prototype[Symbol.matchAll].call('', '');
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  RegExp.prototype[Symbol.matchAll].call(Symbol(), '');
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  RegExp.prototype[Symbol.matchAll].call(1, '');
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}
