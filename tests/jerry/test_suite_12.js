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

(function tc_12_02__007() {
  var a = 'undefined';

  assert(a === 'undefined');
})();

(function tc_12_02__019() {
  var
          a = 2

  assert(a === 2);
})();

(function tc_12_02__022() {
  var a = 1;
  var b = a;

  assert(a === b);
})();

(function tc_12_02__003() {
  var a = null;
  assert(a === null);
})();

(function tc_12_02__001() {
  var a;
  assert(a === undefined);
})();

(function tc_12_02__006() {
  var a = 'undefined';

  assert(a === "undefined");
})();

(function tc_12_02__014() {
  var

          $a = 2;
  assert($a === 2);
})();

(function tc_12_02__010() {
  var a, b = 3.14, c;

  assert(a === undefined && b === 3.14 && c === undefined)
})();

(function tc_12_02__004() {
  var a = undefined;

  assert(a === undefined);
})();

(function tc_12_02__005() {
  var a = "undefined";

  assert(a === "undefined");
})();

(function tc_12_02__009() {
  var a = 1,
          b,
          c = 4;

  b = a + c;

  assert(b === 5);
})();

(function tc_12_02__011() {
  var a, b, c;

  assert(a === undefined && b === undefined && c === undefined);
})();

(function tc_12_02__002() {
  var a = 12345;
  assert(a === 12345);
})();

(function tc_12_02__020() {
  var a = RegExp();

  assert(a instanceof RegExp);
})();

(function tc_12_02__015() {
  var a = 12 + "abc";

  assert(a === "12abc");
})();

(function tc_12_02__018() {
  var a = [];

  assert(a instanceof Array && a.length === 0);
})();

(function tc_12_02__016() {
  var a = {};
  var b = typeof (a);

  assert(b === "object" && b === typeof (Object()) && b === typeof ({}));
})();

(function tc_12_02__008() {
  var a = false, b = true;

  assert(a === false && b === true);
})();

(function tc_12_02__013() {
  var
          _a$1 = 2;

  assert(_a$1 === 2);
})();

(function tc_12_02__012() {
  var
          _a1 = 2;

  assert(_a1 === 2);
})();

(function tc_12_02__021() {
  var a = new Date();

  assert(a instanceof Date);
})();

(function tc_12_02_01__002() {
  var arguments = 2;
  assert(arguments === 2 && typeof (arguments) === "number");
})();

(function tc_12_02_01__001() {
  var eval = 1;
  assert(eval === 1 && typeof (eval) === "number");
})();

(function tc_12_12__006() {
  var x = 0;
  lablemark:
          if (x < 6) {
    ++x;
    break lablemark;
  }

  assert(x !== 6);
})();

(function tc_12_12__010() {
  var x = 0;
  for (i = 0; i < 10; ++i) {
    lablemark:
            for (j = 0; j < 10; ++j) {
      for (k = 0; k < 10; ++k) {
        ++x;
        continue lablemark;
      }
    }
  }

  assert(x == 100)
})();

(function tc_12_12__007() {
  var x = 0;
  for (i = 0; i < 10; ++i) {
    for (j = 0; j < 10; ++j) {
      lablemark:
              for (k = 0; k < 10; ++k) {
        ++x;
        break lablemark;
      }
    }
  }

  assert(x == 100)
})();

(function tc_12_12__003() {
  switchMark:
          switch (1) {
    case 0:
      break;
    case 1:
      break switchMark;
    case 2:
      assert(false);
  }
})();

(function tc_12_12__009() {
  var x = 0;
  lablemark:
          for (i = 0; i < 10; ++i) {
    for (j = 0; j < 10; ++j) {
      for (k = 0; k < 10; ++k) {
        ++x;
        break lablemark;
      }
    }
  }

  assert(x == 1)
})();

(function tc_12_12__008() {
  var x = 0;
  for (i = 0; i < 10; ++i) {
    lablemark:
            for (j = 0; j < 10; ++j) {
      for (k = 0; k < 10; ++k) {
        ++x;
        break lablemark;
      }
    }
  }

  assert(x == 10)
})();

(function tc_12_12__005() {
  whileMark:
          for (i = 0; i < 10; ++i) {
    continue whileMark;
    assert(false);
  }
})();

(function tc_12_12__001() {
  loop:
          while (true) {
    break loop;
  }
})();

(function tc_12_12__004() {
  var i = 0;

  whileMark:
          while (i < 10) {
    ++i;
    continue whileMark;
    assert(false);
  }
})();

(function tc_12_12__002() {
  loop:
          while (true) {
    do {
      for (; true; ) {
        break loop;
      }
    } while (true);
  }
})();

(function tc_12_03__003() {
  var a="ab;c"
  ;
  ;
  assert (a === 'ab;c');
})();

(function tc_12_03__001() {
  function test()
  {
    ;
    return 1;;;;;;
  }

  test();
})();

(function tc_12_03__002() {
  function test()
  {
   ;;;;;;
   return 1
  }

  test();
})();

(function tc_12_05__001() {
  function test()
  {
    if (true) {
      return 1;
    }
    assert(false);
  }

  test();
})();

(function tc_12_05__007() {
  var a = 1, b = 2;
  var c;
  if (a === 1)
  {
    if (b === 1)
      c = 3;
  }
  else
  if (b === 2)
    c = 5;
  else
    c = 7;

  assert(c === undefined);
})();

(function tc_12_05__008() {
  var a = 1, b = 2;
  var c;
  if (a === 1)
    if (b === 1)
      c = 3;
    else
    if (b === 2)
      c = 5;
    else
      c = 7;

  assert(c === 5);

})();

(function tc_12_05__004() {
  function test()
  {
    if (false)
      assert(false);
    else {
      return 1;
    }
  }

  test();
})();

(function tc_12_05__006() {
  var a = 'w\0', b = 'w\0';
  assert(a === b);
})();

(function tc_12_05__002() {
  function test()
  {
    if (false) {
      assert(false);
    }
  }

  test();
})();

(function tc_12_05__005() {
  function test()
  {
      if(true);
  }

  test();
})();

(function tc_12_05__003() {
  function test()
  {
    if (true)
    {
      return 1;
    }
    else
      assert(false);
  }

  test();
})();

(function tc_12_06_03__008() {
  var sum = 1;

  for (var i = 0; i < 10; i++, sum *= i)
    ;

  assert(sum === 3628800);
})();

(function tc_12_06_03__010() {
  for
          (
                  var i = 0
                  ;
                  i < 10
                  ;
                  i++
                  )
  {
    i++;
  }

  assert(i == 10);
})();

(function tc_12_06_03__005() {
  var i = 0;
  for (; ; )
  {
    if (i++ === 100)
      break;
  }
})();

(function tc_12_06_03__004() {
  var i;

  for (i = 1; i < 20; )
    i *= 2;

  assert(i === 32);
})();

(function tc_12_06_03__011() {
  var i;

  function test()
  {
    for (i = 0; i < 10; i++)
    {
      if (i === 4)
        return 1;
    }

    return 0;
  }

  var r = test();

  assert(r === 1 && i === 4);
})();

(function tc_12_06_03__009() {
  var init;

  for (init = (129 - 8) / 11; init != 11; )
  {
    init = 33;
  }

  assert(init == 11);
})();

(function tc_12_06_03__001() {
  var cnt = 1;

  for (var i = 0; i < 10; i++)
    cnt++;

  assert(cnt === 11 && i === 10);
})();

(function tc_12_06_03__006() {
  var sum = 0;
  for (var i = 1; i <= 10; i++)
  {
    for (var j = 1; j <= 5; j++)
    {
      sum += i * j;
    }
  }

  assert(sum === 825);
})();

(function tc_12_06_03__007() {
  var sum = 0;
  for (var i = 1, j = 1, k = 1; i + j + k <= 100; i++, j += 2, k += 3)
  {
    sum += i + j + k;
  }

  assert(sum == 867);
})();

(function tc_12_06_03__003() {
  var i;

  for (i = 0; ; i += 5)
  {
    if (i === 100)
      break;
  }
})();

(function tc_12_06_03__002() {
  var cond = 1.999;
  var cnt = 0;

  for (; cond < 10.333; cond += 1.121)
    cnt++;

  assert(cnt == 8);
})();

(function tc_12_06_04__007() {
  var o = {a: 1, b: 2, c: 3, d: 4, e: 5};

  function test()
  {
    for (var p in o)
    {
      if (p === 'c')
        return 1;

      o[p] += 4;
    }
    return 0;
  }

  var r = test();

  assert(((o.a === 5 && o.b === 6 && o.c === 3) ||
          (o.c === 3 && o.d === 8 && o.e === 9)) && r === 1);
})();

(function tc_12_06_04__006() {
  var o = {a: 1, b: 2, c: 3};

  for
          (
                  var
                  p in o
                  )
  {
    o[p] += 4;
  }

  assert(o.a === 5 && o.b === 6 && o.c === 7);
})();

(function tc_12_06_04__002() {
  var a = new Array(1, 2, 3, 4, 5, 6, 7);
  a.eight = 8;

  var p;
  for (p in a)
  {
    a[p] += 1;
  }

  assert(a[0] === 2 && a[1] === 3 && a[2] === 4 && a[3] === 5 &&
          a[4] === 6 && a[5] === 7 && a[6] === 8 && a['eight'] === 9);
})();

(function tc_12_06_04__005() {
  var b = {basep: "base"};

  function dConstr()
  {
    this.derivedp = "derived";
  }
  dConstr.prototype = b;

  var d = new dConstr();

  for (var p in d)
  {
    d[p] += "A";
  }

  assert(d.basep === "baseA" && d.derivedp === "derivedA");
})();

(function tc_12_06_04__004() {
  var a;

  for (var p in a)
  {
    assert(false);
  }
})();

(function tc_12_06_04__001() {
  var o = {a: 1, b: 2, c: 3};

  for (var p in o)
  {
    o[p] += 4;
  }

  assert(o.a === 5 && o.b === 6 && o.c === 7);
})();

(function tc_12_06_04__003() {
  var a = null;

  for (var p in a)
  {
    assert(false);
  }
})();

(function tc_12_06_01__002() {
  var cnt = 1;
  do
  {
    cnt++;
    if (cnt === 42) {
      break;
    }
  } while (true);
})();

(function tc_12_06_01__010() {
  var cnt = 0;

  function test()
  {
    do
    {
      cnt++;
      if (cnt === 8)
        return 1;
    }
    while (cnt < 10);

    return 0;
  }

  var r = test();

  assert(cnt === 8 && r === 1);
})();

(function tc_12_06_01__009() {
  var cnt = 0;
  do
  {
    cnt++;
  }
  while
          (cnt < 10
                  );

  assert(cnt === 10);
})();

(function tc_12_06_01__006() {
  var cnt = 1;
  do
  {
    cnt++;
  }
  while (false);

  assert(cnt === 2);
})();

(function tc_12_06_01__001() {
  var cnt = 1;
  do
    cnt++;
  while (cnt < 10);

  assert(cnt === 10);
})();

(function tc_12_06_01__007() {
  var cnt = 1;
  do
  {
    cnt++;
  }
  while (!"string");

  assert(cnt === 2);
})();

(function tc_12_06_01__003() {
  var cnt = 1;
  do {
    cnt++;

    if (cnt === 10)
    {
      break;
    }
  }
  while (0, 1);
})();

(function tc_12_06_01__005() {
  var x = 1 / 3;
  do
  {
    x = 1;
  }
  while (x === 3 / 9);
  assert (x === 1);
})();

(function tc_12_06_01__008() {
  var cnt = 0;
  do
  {
    cnt++;
  }
  while (!(cnt & 0x8000));

  assert(cnt == 32768);
})();

(function tc_12_06_01__004() {
  var obj = new Object();
  obj.x = "defined";
  do
  {
    delete obj.x;
  }
  while (obj.x !== undefined);
})();

(function tc_12_06_02__004() {
  var cnt = 33;

  while ("")
    cnt /= 2;

  assert(cnt === 33);
})();

(function tc_12_06_02__005() {
  var bVal = true;
  var val = "test";

  while (!bVal)
  {
    val += " of while";
  }

  assert(val === "test");
})();

(function tc_12_06_02__008() {
  var cnt = 25;

  function test()
  {
    while (cnt)
    {
      cnt--;
      if (cnt === 3)
        return 1;
    }
    return 0;
  }

  r = test();

  assert(cnt === 3 && r === 1);
})();

(function tc_12_06_02__002() {
  var cnt = 0;

  while (1) {
    cnt++;
    if (cnt === 10)
      break;
  }
})();

(function tc_12_06_02__003() {
  var cnt = 0;
  while ((1234 - 34) % 23 - 1234 * 23.222)
  {
    cnt++;
    if (cnt === 10)
      break;
  }
})();

(function tc_12_06_02__001() {
  var cnt = 25;
  while (cnt)
    cnt--;

  assert(cnt === 0);
})();

(function tc_12_06_02__007() {
  var bitField = 0x1000000;
  var cnt = 0;

  while
          (
                  bitField >>= 1 &&
                  true
                  )
  {
    cnt++;
  }

  assert(cnt === 24);
})();

(function tc_12_06_02__006() {
  var bitField = 0x1000000;
  var cnt = 0;

  while (bitField >>= 1)
  {
    cnt++;
  }

  assert(cnt === 24);
})();

(function tc_12_09__002() {
  var r = test()

  assert(r === 1);

  function test()
  {
    return ((23 << 2) + 8) / 100;
  }
})();

(function tc_12_09__001() {
  var r = test()

  assert(typeof r == 'object' && r.prop1 === "property1" && r.prop2 === 2 && r.prop3 === false);

  function test()
  {
    var o = {
      prop1: "property1",
      prop2: 2,
      prop3: false
    };

    return o;
  }
})();

(function tc_12_09__005() {
  var r = test()

  assert(r === 100);

  function test()
  {
    function internal()
    {
      return 100;
    }

    return internal();
  }
})();

(function tc_12_09__006() {
  var r = test()

  assert(typeof r == 'function');

  function test()
  {
    function internal()
    {
      return 100;
    }

    return internal;
  }
})();

(function tc_12_09__004() {
  var r = test()

  assert(r == undefined);

  function test()
  {
    var r = 1;
    return
    r;
  }
})();

(function tc_12_09__003() {
  var r = test()

  assert(r === 33);

  function test()
  {
    return 33;
  }
})();

(function tc_12_04__003() {
  var a, b, c
  a = 2, b = 3, c = 4
})();

(function tc_12_04__001() {
  var a = 5, b = 1;
  (a + b);
})();

(function tc_12_04__002() {
  var a = 5, b = 1;
  a = a && b;
})();

(function tc_12_04__004() {
  var a
  a = function () {
  }
})();

(function tc_12_08__008() {
  var i = 9;
  var cnt = 0;

  while (i-- > 0)
  {
    if (i % 2)
      break;

    var j = 0;
    while (j++ < 20)
    {
      if (j % 2 === 0)
        break;
      cnt++;
    }

  }

  assert(cnt === 1);
})();

(function tc_12_08__014() {
  var sum = 0;
  var i = 0, j = 0;
  top:
          do
  {
    j = 0;

    do
    {
      if (j > 9 && i % 2)
        break top;

      sum += 1;
    }
    while (j++ < 20);

    sum += 1;
  }
  while (i++ < 10);

  assert(sum === 32);
})();

(function tc_12_08__009() {
  var sum = 0, i = 0;

  WhileLabel:
          while (++i < 10)
  {
    if (i === 5)
    {
      break WhileLabel;
    }

    sum += i;
  }

  assert(sum === 10);
})();

(function tc_12_08__010() {
  var sum = 0;
  var i = 0, j = 0;
  top:
          while (i++ < 10)
  {
    j = 0;
    while (j++ < 20)
    {
      if (j > 9 && i % 2)
        break top;

      sum += 1;
    }

    sum += 1;
  }

  assert(sum === 9);
})();

(function tc_12_08__011() {
  var mask = 0xff0f;
  var numOnes = 0;

  do
  {
    if (!(mask & 1))
      break;

    mask >>= 1;
    numOnes++;
  } while (mask);

  assert(numOnes === 4);
})();

(function tc_12_08__002() {
  var o = {p1: 1,
    p2: {p1: 100, p2: 200, p3: 100},
    p3: 4,
    p4: 7,
    p5: 124686,
    p6: {p1: 100, p2: 200, p3: 100},
    p7: 1},
  sum = 0;

  for (var p in o)
  {
    if (p === "p4")
      break;

    if (typeof (o[p]) === "object")
    {
      for (var pp in o[p])
      {
        if (pp === "p2")
          break;

        sum += o[p][pp];
      }
    }
    else
    {
      sum += 20;
    }
  }

  assert(sum === 140);
})();

(function tc_12_08__007() {
  var mask = 0xff0f;
  var numOnes = 0;

  while (mask)
  {
    if (!(mask & 1))
      break;

    mask >>= 1;
    numOnes++;
  }

  assert(numOnes === 4);
})();

(function tc_12_08__004() {
  function main()
  {
    var sum = 0;
    for (var i = 0; i < 10; i++)
      for (var j = 0; j < 20; j++)
      {
        if (j === 10)
          break;

        sum += 1;
      }

    assert(sum === 100);
  }

  main ();
})();

(function tc_12_08__016() {
  var o = {p1: 1, p2: 2, p3: {p1: 150, p2: 200, p3: 130, p4: 20}, p4: 4, p5: 46}, sum = 0;
  for (var p in o)
  {
    if (p === "p4")
      continue;

    if (typeof (o[p]) === "object")
    {
      for (var pp in o[p])
      {
        if (pp === "p2")
          break;

        sum += o[p][pp];
      }
    }
    else {
      sum += o[p];
    }
  }

  assert(sum === 199);
})();

(function tc_12_08__012() {
  var i = 10;
  var cnt = 0;

  do
  {
    var j = 0;
    do
    {
      if (j % 2 === 0)
        break;
      cnt++;
    }
    while (j++ < 20)

    if (i % 2)
      break;
  }
  while (i-- > 0);

  assert(cnt === 0);
})();

(function tc_12_08__001() {
  var o = {p1: 1,
    p2: {p1: 100, p2: 200, p3: 100},
    p3: 4,
    p4: 7,
    p5: 124686,
    p6: {p1: 100, p2: 200, p3: 100},
    p7: 1},
  sum = 0;


  for (var p in o)
  {
    if (p === "p4")
      break;

    if (typeof (o[p]) === "object")
    {
      top:
              for (var pp in o[p])
      {
        if (pp === "p2")
          break top;

        sum += o[p][pp];
      }
    }

    sum += 20;

  }

  assert(sum === 160);
})();

(function tc_12_08__018() {
  var o = {p1: 1,
    p2: {p1: 100, p2: 200, p3: 100},
    p3: 4,
    p4: 7,
    p5: 124686,
    p6: {p1: 100, p2: 200, p3: 100},
    p7: 1},
  sum = 0;

  top:
          for (var p in o)
  {
    if (p === "p4")
      break;

    if (typeof (o[p]) === "object")
    {
      for (var pp in o[p])
      {
        if (pp === "p2")
          break top;

        sum += o[p][pp];
      }
    }

    sum += 20;

  }

  assert(sum === 120)
})();

(function tc_12_08__013() {
  var sum = 0, i = 0;

  DoWhileLabel:
          do
  {
    if (i === 5)
    {
      break DoWhileLabel;
    }

    sum += i;
  }
  while (++i < 10);

  assert(sum === 10);
})();

(function tc_12_08__005() {
  var sum = 0;

  ForLabel:
          for (var i = 0; i < 10; i++)
  {
    if (i === 5)
    {
      break ForLabel;
    }

    sum += i;
  }

  assert(sum === 10);
})();

(function tc_12_08__015() {
  var o = {p1: 1, p2: 2, p3: 3, p4: 1, p5: 2}, sum = 0;
  for (var p in o)
  {
    if (p === "p3")
    {
      break;
    }

    sum += o[p];
  }

  assert(sum === 3);
})();

(function tc_12_08__017() {
  var o = {a: 1, b: 2, c: 3};

  ForLabel:
          for (var p in o)
  {
    if (p === "b")
      break ForLabel;
    o[p] += 4;
  }

  assert(o.a + o.b + o.c === 10);
})();

(function tc_12_08__003() {
  var sum = 0;
  for (var i = 0; i < 10; i++)
  {
    if (i === 5)
    {
      break;
    }

    sum += i;
  }

  assert(sum === 10);
})();

(function tc_12_08__006() {
  var sum = 0;
  top:
          for (var i = 0; i < 10; i++)
  {
    for (var j = 0; j < 20; j++)
    {
      if (j > 9 && i % 2)
        break top;

      sum += 1;
    }

    sum += 1;
  }

  assert(sum === 31);
})();

(function tc_12_10__003() {
  var o = {prop: "property"};

  with (o) {
    assert(prop == "property");
  }
})();

(function tc_12_10__005() {
  with ({})
  {
    var x = "abc";
  }

  assert(x === "abc");
})();

(function tc_12_10__004() {
  var x;

  function test()
  {
    with (Math)
    {
      x = abs(-396);
      return 1;
    }

    return 0;
  }

  var r = test();

  assert(r === 1 && x === 396);
})();

(function tc_12_10__002() {
  var o = {prop: "property"};

  with (o) {
    assert(prop == "property");
  }
})();

(function tc_12_10__007() {
  var x, y;

  with
          (
                  Math)
  {
    x = cos(PI);
  }

  assert(x == -1);
})();

(function tc_12_10__001() {
  var x, y;

  with (Math) {
    x = cos(PI);
  }

  assert(x == -1);
})();

(function tc_12_10__006() {
  assert(test("hello") == "hello" && typeof test({}) == "object" && test({arg: "hello"}) == "hello")

  function test(arg) {
    with (arg) {
      return arg;
    }
  }
})();

(function tc_12_11__005() {
  switch (1) {
  }
})();

(function tc_12_11__002() {
  var matchesCount = 0;

  switch ("key") {
    case "key":
      ++matchesCount;
      break;
    case "key":
      ++matchesCount;
      break;
    case "another key":
      ++matchesCount;
      break;
    default:
      ++matchesCount;
      break;
  }

  assert (matchesCount === 1);
})();

(function tc_12_11__001() {
  switch (1) {
    case 0:
      assert(false);
    case 1:
      break;
    default:
      assert(false);
  }
})();

(function tc_12_11__006() {
  function fact(n)
  {
    return n < 2 ? 1 : n * fact(n - 1);
  }

  switch (fact(5)) {
    case 5 * fact(4):
      break;
    default:
      assert(false);
  }
})();

(function tc_12_11__004() {
  var counter = 0;

  switch ("key") {
    case "key":
      ++counter;
    case "another key":
      ++counter;
    case "another key2":
      ++counter;
    default:
      ++counter;
  }

  assert (counter == 4);
})();

(function tc_12_11__003() {
  switch (1) {
    case true:
      assert(false);
    case false:
      assert(false);
    default:
      assert(true);
  }
})();

(function tc_12_11__007() {
  switch ("key") {
    case "another key":
      assert(false);
    default:
      break;
    case "another key2":
      assert(false);
  }
})();

(function tc_12_14__001() {
  try {
    try {
      throw "error";
    } catch (e) {
      throw e;
    }
    assert(false);
  } catch (e) {
  }
})();

(function tc_12_14__003() {
  function test ()
  {
      try {
          throw 1;
      } catch (e) {
          return (e === 1);
      }

      return false;
  }

  assert (test ());
})();

(function tc_12_14__004() {
  function test ()
  {
    try {
      throw "error";
    } catch (e) {
      return false;
    } finally {
      return true;
    }

    return false;
  }

  assert (test ());
})();

(function tc_12_14__006() {
  function test ()
  {
    try {
      var x = 1;
    } finally {
      return true;
    }

    return false;
  }

  assert (test ());
})();

(function tc_12_14__005() {
  function test ()
  {
      try {
          throw "error";
      } catch (e) {
          return true;
      } finally {
      }

      return false;
  }

  assert (test ());
})();

(function tc_12_14__002() {
  function test ()
  {
    try {
      var x = 1;
    } catch (e) {
      return false;
    }

    return true;
  }

  assert (test ());
})();

(function tc_12_01__005() {
  {
    {
      var a = null;
      ;
    }
    {
      {
      }
    }
    a = 'null';
  }

  assert(a === 'null');
})();

(function tc_12_01__004() {
  {
    var a = null;
    ;
    a = 'null';
  }

  assert(a === 'null');
})();

(function tc_12_01__002() {
  function test ()
  {
    {
      return true;
    }
    return false;
  }

  assert (test ());
})();

(function tc_12_01__003() {
  {;;}
})();

(function tc_12_01__001() {
  {
  }
})();

(function tc_12_13__001() {
  function test ()
  {
    try {
      if (true) {
        throw "error";
      }
    } catch (e) {
      return true;
    }
    return false;
  }

  assert (test ());
})();

(function tc_12_13__003() {
  function d () {
    throw "exception";
  }
  function c () {
    d ();
  }
  function b () {
    c ();
  }
  function a () {
    b ();
  }

  function test ()
  {
    try {
      a ();
    } catch (e) {
      return true;
    }
    return false;
  }

  assert (test ());
})();

(function tc_12_13__002() {
  function test ()
  {
    try {
      while (true) {
        throw "error";
      }
    } catch (e) {
      return true;
    }
    return false;
  }

  assert (test ());
})();

(function tc_12_07__001() {
  var sum = 0;
  for (var i = 0; i < 10; i++)
  {
    if (i === 5)
    {
      continue;
    }

    sum += i;
  }

  assert(sum === 40);
})();

(function tc_12_07__009() {
  var sum = 0, i = 0;

  WhileLabel:
          while (++i < 10)
  {
    if (i === 5)
    {
      continue WhileLabel;
    }

    sum += i;
  }

  assert(sum === 40);
})();

(function tc_12_07__006() {
  var sum = 0;
  top:
          for (var i = 0; i < 10; i++)
  {
    for (var j = 0; j < 20; j++)
    {
      if (j > 9 && i % 2)
        continue top;

      sum += 1;
    }

    sum += 1;
  }

  assert(sum === 155);
})();

(function tc_12_07__002() {
  var sum = 0;
  for (var i = 0; i < 10; i++)
    for (var j = 0; j < 20; j++)
    {
      if (j > 9)
        continue;

      sum += 1;
    }

  assert(sum === 100);
})();

(function tc_12_07__015() {
  var o = {p1: 1, p2: 2, p3: 3, p4: 4, p5: 5}, sum = 0;
  for (var p in o)
  {
    if (p == "p3")
    {
      continue;
    }

    sum += o[p];
  }

  assert(sum == 12)
})();

(function tc_12_07__008() {
  var i = 10;
  var cnt = 0;

  while (i-- > 0)
  {
    if (i % 2)
      continue;

    var j = 0;
    while (j++ < 20)
    {
      if (j % 2 === 0)
        continue;
      cnt++;
    }

  }

  assert(cnt === 50);
})();

(function tc_12_07__004() {
  var o = {a: 1, b: 2, c: 3};

  ForLabel:
          for (var p in o)
  {
    if (p === "b")
      continue ForLabel;
    o[p] += 4;
  }

  assert(o.a === 5 && o.b === 2 && o.c === 7);
})();

(function tc_12_07__012() {
  var i = 10;
  var cnt = 0;

  do
  {
    if (i % 2)
      continue;

    var j = 0;
    do
    {
      if (j % 2 === 0)
        continue;
      cnt++;
    }
    while (j++ < 20)
  }
  while (i-- > 0);

  assert(cnt === 60);
})();

(function tc_12_07__003() {
  var o = {p1: 1, p2: 2, p3: {p1: 100, p2: 200, p3: 100}, p4: 4, p5: 5}, sum = 0;

  top:
          for (var p in o)
  {
    if (p === "p2")
      continue;

    if (typeof (o[p]) === "object")
    {
      for (var pp in o[p])
      {
        if (pp === "p2")
          continue top;

        sum += o[p][pp];
      }
    }

    sum += 20;

  }

  assert(sum === 160);
})();

(function tc_12_07__016() {
  var o = {p1: 1, p2: 2, p3: {p1: 100, p2: 200}, p4: 4, p5: 5}, sum = 0;
  for (var p in o)
  {
    if (p === "p2")
      continue;

    if (typeof (o[p]) === "object")
    {
      for (var pp in o[p])
      {
        if (pp === "p2")
          continue;

        sum += o[p][pp];
      }
    }
    else {
      sum += o[p];
    }
  }

  assert(sum === 110);
})();

(function tc_12_07__013() {
  var sum = 0, i = 0;

  DoWhileLabel:
          do
  {
    if (i === 5)
    {
      continue DoWhileLabel;
    }

    sum += i;
  }
  while (++i < 10);

  assert(sum === 40);
})();

(function tc_12_07__010() {
  var sum = 0;
  var i = 0, j = 0;
  top:
          while (i++ < 10)
  {
    j = 0;
    while (j++ < 20)
    {
      if (j > 9 && i % 2)
        continue top;

      sum += 1;
    }

    sum += 1;
  }

  assert(sum === 150);
})();

(function tc_12_07__007() {
  var mask = 0xff0f;
  var numZeroes = 0;

  while (mask)
  {
    mask >>= 1;

    if (mask & 1)
      continue;

    numZeroes++;
  }

  assert(numZeroes === 5);
})();

(function tc_12_07__011() {
  var mask = 0xff0f;
  var numZeroes = 0;

  do
  {
    mask >>= 1;

    if (mask & 1)
      continue;

    numZeroes++;
  } while (mask);

  assert(numZeroes === 5);
})();

(function tc_12_07__005() {
  var sum = 0;

  ForLabel:
          for (var i = 0; i < 10; i++)
  {
    if (i === 5)
    {
      continue ForLabel;
    }

    sum += i;
  }

  assert(sum === 40);
})();

(function tc_12_07__014() {
  var sum = 0;
  var i = 0, j = 0;
  top:
          do
  {
    j = 0;

    do
    {
      if (j > 9 && i % 2)
        continue top;

      sum += 1;
    }
    while (j++ < 20);

    sum += 1;
  }
  while (i++ < 10);

  assert(sum === 182);
})();
