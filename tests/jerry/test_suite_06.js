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

(function tc_06__004() {
  var arg = 3;
  function a() {
    return 5 + arg;
  }

  arg = 4;
  var b = function () {
    return 6 + arg;
  };

  arg = 5;
  c = function e() {
    return 7 + arg;
  };

  assert(a() + b() + c() === 33);
})();

(function tc_06__005() {
  var a = "\u0410\u0411";
  var b = "\u0509\u0413";

  assert(a < b);
})();

(function tc_06__003() {
  var obj = new Object();

  function c(arg)
  {
    var obj = new Object();
    obj.par = arg;
    obj.print = function () {
      return arg;
    };
    return obj;
  }

  var a = c(5);
  var b = c(6);
  assert(a.print() + b.par === 11);
})();

(function tc_06__001() {
  var str = "a\u000Ab";
  assert(str[1] === '\n');
})();

(function tc_06__002() {
  function c(arg)
  {
    var obj = new Object();
    obj.print = function () {
      f = arg;
    };
    return obj;
  }

  a = c(5);
  b = c(6);

  a.print.toString = 7;

  assert(typeof a.print.toString !== typeof b.print.toString);
})();
