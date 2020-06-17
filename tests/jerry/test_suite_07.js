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

(function tc_07_09__002() {
  function test()
  {
    var a = 1, b = 2;

    return
    a + b
  }

  var v = test();

  assert(v !== 3);

  assert(typeof v === "undefined")
})();

(function tc_07_09__008() {
  function test()
  {
    var a = 10, b = 5;
    var c = a + b

    return c;
  }

  assert(test() == 15);
})();

(function tc_07_09__004() {
  var obj = new Object();

  function c(arg)
  {
    var obj = new Object();
    obj.par = arg;
    obj.print = function () {
      return arg;
    }
    return obj;
  }

  var a, b = 1, d = 2, e = 3;

  a = b + c
          (d + e).print()

  assert(a === 6);
})();

(function tc_07_09__005() {
  var b = 4, c = 5;

  a = b
  --c

  assert(a === 4 && c === 4);
})();

(function tc_07_09__007() {
  var mainloop = 1, cnt = 0;

  for (var i = 0; i < 10; ++i)
  {
    for (var j = 0; j < 10; ++j)
    {
      if (j == 6)
      {
        break
        mainloop
      }

      ++cnt;
    }
  }

  assert(cnt == 60);
})();

(function tc_07_09__009() {
  {
    var a, b = 3, c = 30;
    a = b + c}

  assert (a == 33);
})();

(function tc_07_09__010() {
  assert (glob === undefined);

  var glob = 34

  assert (glob === 34);
})();

(function tc_07_09__003() {
  var b = 4, c = 5;

  a = b
  ++c

  assert(a === 4 && c === 6);
})();

(function tc_07_09__001() {
  { 1
  2 } 3
})();

(function tc_07_09__006() {
  var mainloop = 1, cnt = 0;

  for (var i = 0; i < 10; ++i)
  {
    for (var j = 0; j < 10; ++j)
    {
      if (j == 6)
      {
        continue
        mainloop
      }

      ++cnt;
    }
  }

  assert(cnt == 90);
})();

(function tc_07_06_01__001() {
  var package = 1;
})();

(function tc_07_08_05__001() {
  /a[a-z]/.exec("abcdefghi");
})();
