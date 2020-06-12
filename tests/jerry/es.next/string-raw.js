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

function assertThrows(str, error) {
  try {
    eval(str);
    assert(false);
  } catch (e) {
    if (typeof error === "function") {
      assert(e instanceof error);
    } else {
      assert(e === error)
    }
  }
}

// https://www.ecma-international.org/ecma-262/6.0/#sec-string.raw 21.1.2.4
// Note: the string error messages represents the nth step of the routine
var abruptTests = [
  ["undefined", TypeError], // 3.
  ["{ get raw() { throw '5'; } }", '5'],
  ["{ raw : undefined }", TypeError], // 5.toObject
  ["{ raw : { get length() { throw '7.1'; } } }", '7.1'],
  ["{ raw : { length : { toString() { throw '7.2'; } } } }", '7.2'],
  ["{ raw : { length: 2, get '0'() { throw '12.b.1'} } }", '12.b.1'],
  ["{ raw : { length: 2, '0' : { toString() { throw '12.b.2';} } } }", '12.b.2'],
  ["{ raw : { length: 2, '0' : 1 } }, { toString() { throw '12.h';} }", '12.h'],
];

abruptTests.forEach(e => {
  assertThrows("String.raw(" + e[0] + ")", e[1]);
});

// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String/raw
assert(String.raw`Hi\n${2+3}!` === 'Hi\\n5!');
assert(String.raw`Hi\u000A!` === 'Hi\\u000A!');

let name = 'Bob';
assert(String.raw`Hi\n${name}!` === "Hi\\nBob!");

let str = String.raw({
  raw: ['foo', 'bar', 'baz']
}, 2 + 3, 'Java' + 'Script');
assert(str === "foo5barJavaScriptbaz");

assert(String.raw({ raw: 'test' }, 0, 1, 2) === "t0e1s2t");

var get = [];
var raw = new Proxy({length: 2, 0: '', 1: ''}, { get: function(o, k) { get.push(k); return o[k]; }});
var p = new Proxy({raw: raw}, { get: function(o, k) { get.push(k); return o[k]; }});
String.raw(p);
assert(get + '' === "raw,length,0,1");

assert(String.raw`\\` == "\\\\")
assert(String.raw`\`` == "\\`")
assert(String.raw`\
\
` == "\\\n\\\n")
assert(String.raw`\ ` == "\\\u2029")
