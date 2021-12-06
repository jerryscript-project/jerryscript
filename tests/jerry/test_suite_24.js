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

(function tc_24_01_02__010() {
  var a = new ArrayBuffer(NaN);
  assert(a.byteLength === 0);
})();

(function tc_24_01_02__009() {
  var a = new ArrayBuffer(undefined);
  assert(a.byteLength === 0);
})();

(function tc_24_01_02__012() {
  var name = "";
  try
  {
    var a = new ArrayBuffer(-1.9);
  }
  catch (e)
  {
    name = e.name;
  }

  assert(name === "RangeError");
})();

(function tc_24_01_02__003() {
  var a = new ArrayBuffer("5");
  assert(typeof a === 'object');
  assert(a.byteLength === 5);
})();

(function tc_24_01_02__013() {
  var name = "";
  try
  {
    var a = new ArrayBuffer(Math.pow(2, 32) - 1);
  }
  catch (e)
  {
    name = e.name;
  }

  assert(name === "RangeError");
})();

(function tc_24_01_02__002() {
  var a = new ArrayBuffer(5);
  assert(typeof a === 'object');
  assert(a.byteLength === 5);
})();

(function tc_24_01_02__007() {
  var a = new ArrayBuffer("string");
  assert(a.byteLength === 0);
})();

(function tc_24_01_02__006() {
  var a = new ArrayBuffer(5.5);
  assert(a.byteLength === 5);
})();

(function tc_24_01_02__004() {
  var name = "";

  try
  {
    var a = ArrayBuffer();
  }
  catch (e)
  {
    name = e.name;
  }

  assert(name === "TypeError");
})();

(function tc_24_01_02__005() {
  assert(ArrayBuffer.length === 1);
})();

(function tc_24_01_02__001() {
  var a = new ArrayBuffer();
  assert(typeof a === 'object');
  assert(a.byteLength === 0);
})();

(function tc_24_01_02__008() {
  var obj = {};
  var a = new ArrayBuffer(obj);
  assert(a.byteLength === 0);
})();

(function tc_24_01_02__011() {
  var a = new ArrayBuffer(-0.3);
  assert(a.byteLength === 0);

  var b = new ArrayBuffer(-0.9);
  assert(b.byteLength === 0);
})();

(function tc_24_01_04__002() {
  var a = new ArrayBuffer(5);
  assert(a.byteLength === 5);
})();

(function tc_24_01_04__006() {
  var a = new ArrayBuffer(5);
  var b = a.slice (3, 2);
  assert(b.byteLength === 0);
})();

(function tc_24_01_04__001() {
  assert(ArrayBuffer.prototype.constructor === ArrayBuffer);
})();

(function tc_24_01_04__004() {
  var a = new ArrayBuffer(5);
  var b = a.slice (1, 3);
  assert(b.byteLength === 2);
})();

(function tc_24_01_04__003() {
  var a = new ArrayBuffer(5);
  a.byteLength = 10;
  assert(a.byteLength === 5);
})();

(function tc_24_01_04__007() {
  var a = new ArrayBuffer(5);
  var b = a.slice();
  assert(b.byteLength === 5);
})();

(function tc_24_01_04__005() {
  var a = new ArrayBuffer(5);
  var b = a.slice(1, -2);
  assert(b.byteLength === 2);
})();

(function tc_24_01_03__001() {
  assert(typeof ArrayBuffer.prototype === 'object');
})();
