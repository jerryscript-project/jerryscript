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

// Copyright (C) 2019 Leo Balter. All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

assert(String.prototype.replaceAll.length === 2);
var desc = Object.getOwnPropertyDescriptor(String.prototype.replaceAll, "length");
assert(!desc.enumerable);
assert(!desc.writable);
assert(desc.configurable);

assert(String.prototype.replaceAll.name === "replaceAll");
var desc = Object.getOwnPropertyDescriptor(String.prototype.replaceAll, "name");
assert(!desc.enumerable);
assert(!desc.writable);
assert(desc.configurable);

/**
 * Note: The RegExp based replaceAll works the same as the replace method except one special case,
 * when the regexp argument doesn't have a global flag, because then it throws an error
 */

try {
  "foo".replaceAll(/./);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  "foo".replaceAll(/./i);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  "foo".replaceAll(/./m);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  "foo".replaceAll(/./u);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  "foo".replaceAll(/./y);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

var regexp = /a/;
Object.defineProperty(regexp, 'flags', {
  value: 'muyi'
});

try {
  "foo".replaceAll(regexp);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

var regexp = /a/g;
Object.defineProperty(regexp, 'flags', { value: undefined });

try {
  "foo".replaceAll(regexp);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// test basic functionality
var str = "foobarfoo";
assert(str.replaceAll("foo", "bar") === "barbarbar");

var str = "ab c ab cdab cab c"
assert(str.replaceAll("ab c", "z") === "z zdzz");

var str = "ab c"
assert(str.replaceAll("ab c", "z") === "z");

var str = "ab c "
assert(str.replaceAll("ab c", "z") === "z ");

assert('aaab a a aac'.replaceAll('aa', 'z') === 'zab a a zc');
assert('aaab a a aac'.replaceAll('aa', 'a') === 'aab a a ac');
assert('aaab a a aac'.replaceAll('a', 'a') === 'aaab a a aac');
assert('aaab a a aac'.replaceAll('a', 'z') === 'zzzb z z zzc');

function replaceValue() {
  throw 42;
}

assert('a'.replaceAll('b', replaceValue) === "a");
assert('a'.replaceAll('aa', replaceValue) === "a")

assert('aab c  \nx'.replaceAll('', '_') === '_a_a_b_ _c_ _ _\n_x_');
assert('a'.replaceAll('', '_') === '_a_');

// test with replacement string
var str = 'Ninguém é igual a ninguém. Todo o ser humano é um estranho ímpar.';
assert(str.replaceAll('ninguém', '$$') ==='Ninguém é igual a $. Todo o ser humano é um estranho ímpar.');
assert(str.replaceAll('é', '$$') === 'Ningu$m $ igual a ningu$m. Todo o ser humano $ um estranho ímpar.');
assert(str.replaceAll('é', '$$ -') === 'Ningu$ -m $ - igual a ningu$ -m. Todo o ser humano $ - um estranho ímpar.');
assert(str.replaceAll('é', '$$&') === 'Ningu$&m $& igual a ningu$&m. Todo o ser humano $& um estranho ímpar.');
assert(str.replaceAll('é', '$$$') === 'Ningu$$m $$ igual a ningu$$m. Todo o ser humano $$ um estranho ímpar.');
assert(str.replaceAll('é', '$$$$') === 'Ningu$$m $$ igual a ningu$$m. Todo o ser humano $$ um estranho ímpar.');

assert(str.replaceAll('ninguém', '$&') === 'Ninguém é igual a ninguém. Todo o ser humano é um estranho ímpar.');
assert(str.replaceAll('ninguém', '($&)') === 'Ninguém é igual a (ninguém). Todo o ser humano é um estranho ímpar.');
assert(str.replaceAll('é', '($&)') === 'Ningu(é)m (é) igual a ningu(é)m. Todo o ser humano (é) um estranho ímpar.');
assert(str.replaceAll('é', '($&) $&') === 'Ningu(é) ém (é) é igual a ningu(é) ém. Todo o ser humano (é) é um estranho ímpar.');

assert(str.replaceAll('ninguém', '$\'') === 'Ninguém é igual a . Todo o ser humano é um estranho ímpar.. Todo o ser humano é um estranho ímpar.');
assert(str.replaceAll('.', '--- $\'') === 'Ninguém é igual a ninguém---  Todo o ser humano é um estranho ímpar. Todo o ser humano é um estranho ímpar--- ');
assert(str.replaceAll('é', '($\')') === 'Ningu(m é igual a ninguém. Todo o ser humano é um estranho ímpar.)m ( igual a ninguém. Todo o ser humano é um estranho ímpar.) igual a ningu(m. Todo o ser humano é um estranho ímpar.)m. Todo o ser humano ( um estranho ímpar.) um estranho ímpar.');
assert(str.replaceAll('é', '($\') $\'') === 'Ningu(m é igual a ninguém. Todo o ser humano é um estranho ímpar.) m é igual a ninguém. Todo o ser humano é um estranho ímpar.m ( igual a ninguém. Todo o ser humano é um estranho ímpar.)  igual a ninguém. Todo o ser humano é um estranho ímpar. igual a ningu(m. Todo o ser humano é um estranho ímpar.) m. Todo o ser humano é um estranho ímpar.m. Todo o ser humano ( um estranho ímpar.)  um estranho ímpar. um estranho ímpar.');

assert(str.replaceAll('ninguém', '$`') === 'Ninguém é igual a Ninguém é igual a . Todo o ser humano é um estranho ímpar.');
assert(str.replaceAll('Ninguém', '$`') === ' é igual a ninguém. Todo o ser humano é um estranho ímpar.');
assert(str.replaceAll('ninguém', '($`)') === 'Ninguém é igual a (Ninguém é igual a ). Todo o ser humano é um estranho ímpar.');
assert(str.replaceAll('é', '($`)') === 'Ningu(Ningu)m (Ninguém ) igual a ningu(Ninguém é igual a ningu)m. Todo o ser humano (Ninguém é igual a ninguém. Todo o ser humano ) um estranho ímpar.');
assert(str.replaceAll('é', '($`) $`') === 'Ningu(Ningu) Ningum (Ninguém ) Ninguém  igual a ningu(Ninguém é igual a ningu) Ninguém é igual a ningum. Todo o ser humano (Ninguém é igual a ninguém. Todo o ser humano ) Ninguém é igual a ninguém. Todo o ser humano  um estranho ímpar.');

// test when functional replacer toString throws error
function custom() {
  return {
    toString() {
      throw 42;
    }
  }
}

try {
  'a'.replaceAll('a', custom);
  assert(false);
} catch (e) {
  assert(e === 42);
}

function symbol() {
  return {
    toString() {
      return Symbol();
    }
  }
}
  
try {
  'a'.replaceAll('a', symbol);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// test when functional replacer Symbol.replace throws error
var poisoned = 0;
var poison = {
  toString() {
    poisoned += 1;
    throw 'Should not call toString on this/replaceValue';
  },
};

var searchValue = {
  [Symbol.match]: false,
  flags: 'g',
  [Symbol.replace]() {
    throw 42;
  },
  toString() {
    throw 'Should not call toString on searchValue';
  }
};

try {
  ''.replaceAll.call(poison, searchValue, poison);
  assert(false);
} catch (e) {
  assert(e === 42);
}

assert(poisoned === 0);

// test when flags value is undefined or null
var poisoned = 0;
var poison = {
  toString() {
    poisoned += 1;
    throw 'Should not call toString on this/replaceValue';
  },
};

var called = 0;
var value = undefined;
var searchValue = {
  [Symbol.match]: true,
  get flags() {
    called += 1;
    return value;
  }
};

try {
  ''.replaceAll.call(poison, searchValue, poison);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

assert(called === 1) // 1);

called = 0;
value = null;

try {
  ''.replaceAll.call(poison, searchValue, poison);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

assert(called === 1);
assert(poisoned === 0);

// test when Symbol.match throws error
var searchValue = {
  get [Symbol.match]() {
    throw 42;
  }
};

try {
  ''.replaceAll.call(poison, searchValue, poison);
  assert(false);
} catch (e) {
  assert(e === 42);
}
