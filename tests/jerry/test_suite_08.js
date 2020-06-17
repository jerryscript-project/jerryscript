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

(function tc_08_02__001() {
  var x = null;
})();

(function tc_08_02__002() {
  assert(typeof null == 'object');
})();

(function tc_08_05__002() {
  assert(typeof -Infinity == 'number');
})();

(function tc_08_05__001() {
  a = 0x3e7;
  assert(a == 999);
})();

(function tc_08_05__003() {
  assert(0 > -Infinity);
})();

(function tc_08_04__009() {
  var str = "";
  var strObj = new String("");
  var strObj_ = new String();

  assert(str.constructor === strObj.constructor);
})();

(function tc_08_04__007() {
  var str = 'ABC';
  var strObj = new String('ABC');
  assert(str == strObj);
})();

(function tc_08_04__005() {
  var s = 'hello';
  assert(s[5] == undefined);
})();

(function tc_08_04__001() {
  a = '';
  assert(typeof a == "string");
})();

(function tc_08_04__017() {
  var __str__ = "\u0041\u0042\u0043" + 'ABC'
  assert(__str__ === 'ABCABC');
})();

(function tc_08_04__004() {
  var s = 'hello';
  assert(s[0] == 'h');
})();

(function tc_08_04__016() {
  var str = "";
  var strObj = new String;

  assert(typeof str != typeof strObj);
})();

(function tc_08_04__015() {
  var str = "";
  var strObj = new String;

  assert(str !== strObj);
})();

(function tc_08_04__014() {
  var str = "";
  var strObj = new String;

  assert(str == strObj);
})();

(function tc_08_04__003() {
  var str = "test";
  assert(str.constructor === String);
})();

(function tc_08_04__002() {
  assert(("x\0a" < "x\0b") && ("x\0b" < "x\0c"));
})();

(function tc_08_04__010() {
  var str = "";
  var strObj = new String("");
  var strObj_ = new String();

  assert(str.constructor === strObj_.constructor);
})();

(function tc_08_04__008() {
  var str = 'ABC';
  var strObj = new String('ABC');

  assert(str !== strObj);
})();

(function tc_08_04__011() {
  var str = "";
  var strObj = new String("");
  var strObj_ = new String();

  assert(str == strObj);
})();

(function tc_08_04__012() {
  var str = "";
  var strObj = new String("");
  var strObj_ = new String();

  assert(str !== strObj);
})();

(function tc_08_04__013() {
  var str = "";
  var strObj = new String;

  assert(str.constructor === strObj.constructor);
})();

(function tc_08_04__006() {
  var str = 'ABC';
  var strObj = new String('ABC');
  assert(str.constructor === strObj.constructor);
})();

(function tc_08_01__011() {
  assert (test ());

  function test (arg)
  {
    if (typeof (arg) === "undefined")
      return true;
    else
      return false;
  }
})();

(function tc_08_01__009() {
  var x;
  assert(test1() === void 0);

  function test1(x) {
    return x;
  }
})();

(function tc_08_01__008() {
  var x;
  assert(x === void 0);
})();

(function tc_08_01__010() {
  assert (test ());

  function test (arg)
  {
    if (typeof (arg) === "undefined")
      return true;
    else
      return false;
  }
})();

(function tc_08_01__001() {
  var a;
  assert(typeof (a) === "undefined");
})();

(function tc_08_01__006() {
  assert(typeof (void 0) === "undefined");
})();

(function tc_08_01__002() {
  var o = {};

  assert(typeof (o.empty) === "undefined");
})();

(function tc_08_01__003() {
  var a;
  var b = null;

  assert(a == b);
})();

(function tc_08_01__005() {
  a = foo();

  assert(typeof (a) === "undefined");

  function foo() {
  }
})();

(function tc_08_01__007() {
  assert(undefined === void 0);
})();

(function tc_08_01__004() {
  var a;
  assert(!a);
})();

(function tc_08_03__003() {
  assert(!(false == true));
})();

(function tc_08_03__001() {
  var a = true;
  assert(a);
})();

(function tc_08_03__002() {
  var a = false;
  assert(!a);
})();

(function tc_08_03__004() {
  assert(!(false === true));
})();

(function tc_08_12_02__001() {
  var prot = {
    b: 3
  };

  function Custom() {
  }

  Custom.prototype = prot;

  var obj = new Custom();

  assert(obj.b === 3);
})();
