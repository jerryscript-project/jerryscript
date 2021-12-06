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

// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Template_literals
// Tagged templates
var person = 'Mike';
var age = 28;

function myTag(strings, personExp, ageExp) {
  assert(strings[0] == "That ");
  assert(strings[1] == " is a ");
  var str0 = strings[0];
  var str1 = strings[1];

  var ageStr;
  if (ageExp > 99){
    ageStr = 'centenarian';
  } else {
    ageStr = 'youngster';
  }

  return `${str0}${personExp}${str1}${ageStr}`;
}

var output = myTag`That ${ person } is a ${ age }`;
assert(output === "That Mike is a youngster");

function template(strings, ...keys) {
  return (function(...values) {
    var dict = values[values.length - 1] || {};
    var result = [strings[0]];
    keys.forEach(function(key, i) {
      var value = Number.isInteger(key) ? values[key] : dict[key];
      result.push(value, strings[i + 1]);
    });
    return result.join('');
  });
}

var t1Closure = template`${0}${1}${0}!`;
assert(t1Closure('Y', 'A') === "YAY!");
var t2Closure = template`${0} ${'foo'}!`;
assert(t2Closure('Hello', {foo: 'World'}) === "Hello World!");

// Raw strings
(function () {
  function tag(strings) {
    assert(strings.raw[0].length === 40);
  }

  tag`string text line 1 \n string text line 2`;
})();

assert (String.raw`Hi\n${2+3}!` === "Hi\\n5!");

(function () {
  function empty(strings, ...params) {
    assert(strings.length === 4);
    assert(strings.raw.length === 4);
    assert(params.length === 3);
    strings.forEach ((e) => assert (e === ""));
    strings.raw.forEach ((e) => assert (e === ""));
    params.forEach ((e) => assert (e === 1));
  }

  empty`${1}${1}${1}`;
})();

(function () {
  function f (str) {
    return str.raw[0].length;
  }
  assert (eval("f`a\u2029b`") === 3);
})();

(function () {
  function testRaw(parts, a, b) {
    assert(parts instanceof Array);
    assert(parts.raw instanceof Array);
    assert(parts.length === 3);
    assert(parts[0] === "str");
    assert(parts[1] === "escaped\n");
    assert(parts[2] === "");
    assert(parts.raw.length === 3);
    assert(parts.raw[0] === "str");
    assert(parts.raw[1] === "escaped\\n");
    assert(parts.raw[2] === "");
    assert(a === 123);
    assert(b === 456);
    return true;
  }

  assert(testRaw `str${123}escaped\n${456}` === true);
})();

// TemplateStrings call site caching
(function () {
  let str = (arr) => arr;
  function getStr() {
    return str`foo`;
  }
  var chainedCall = getStr();
  var localCall = str`foo`;
  assert(chainedCall === getStr() && chainedCall !== localCall);
})();

// TemplateStrings permanent caching
(function () {
  let str = (arr) => arr;
  function getStr() {
    return str`foo`;
  }
  var chainedCall = getStr();
  var localNew = new getStr();
  assert(chainedCall === getStr() && chainedCall === localNew);
})();

var templateObject;

(function(p) {
  templateObject = p;
})`str`;

var desc = Object.getOwnPropertyDescriptor(templateObject, '0');
assert(desc.writable === false);
assert(desc.enumerable === true);
assert(desc.configurable === false);

(function () {
  function f (strings, ...args) {
    return function () {
      return Array(...args);
    };
  }

  var a = new f`${1}${2}${3}`;
  assert(a.length === 3);
  assert(a[0] === 1);
  assert(a[1] === 2);
  assert(a[2] === 3);

  function g (strings, ...args) {
    return Array;
  }

  a = new g`${1}${2}${3}`(4, 5, 6);
  assert(a.length === 3);
  assert(a[0] === 4);
  assert(a[1] === 5);
  assert(a[2] === 6);

  try {
    new (g`${1}${2}${3}`(4, 5, 6));
    assert(false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  function h (strings, ...args) {
    return 5;
  }

  try {
    new h`foo`;
    assert(false);
  } catch (e) {
    assert (e instanceof TypeError);
  }
})();
