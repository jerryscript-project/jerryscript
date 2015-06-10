// Copyright 2015 Samsung Electronics Co., Ltd.
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

var f = new Function ('');
assert (f () === undefined);

var f = new Function ('"use strict"; f = undefined;');
assert (f () === undefined && f === undefined);

var f = new Function ('"use strict"; q = undefined;');
try
{
  f ();
  assert (false);
}
catch (e)
{
  assert (e instanceof ReferenceError);
}

for (i = 1; i < 10; i ++)
{
  var f = new Function ('a', 'b', 'var q = a; b++; function f (k) {return q + k + b++;}; return f;');

  var fns = new Array ();

  for (var n = 0; n < 10; n++)
  {
    var r = f (1, 7);
    fns[n] = r;

    var check_value = 10;

    for (var m = 0; m < 100; m++)
    {
      var value = r (1);
      assert (check_value++ === value);
    }
  }

  var check_value = 109;
  for (var n = 0; n < 11; n++)
  {
    for (var m = 0; m < 10; m++)
    {
      var value = fns [m] (m * n);
      assert (value == check_value + m * n);
    }

    check_value++;
  }
}

try
{
  new Function ({
    toString : function () {
      throw new TypeError();
    },
    valueOf : function () {
      throw new TypeError();
    }
  });

  assert (false);
}
catch (e)
{
  assert (e instanceof TypeError);
}
