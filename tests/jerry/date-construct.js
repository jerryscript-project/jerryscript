// Copyright 2015-2016 Samsung Electronics Co., Ltd.
// Copyright 2015-2016 University of Szeged.
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

assert (Date.length == 7);
assert (Object.prototype.toString.call (Date.prototype) === '[object Date]');

var d;

try
{
  d = new Date({toString: function() { throw new Error("foo"); }});
  assert (false);
}
catch (e)
{
  assert (e instanceof Error);
  assert (e.message === "foo");
}

assert (isNaN(Date.prototype.valueOf.call(Date.prototype)));

d = new Date("abcd");
assert (isNaN(d.valueOf()));

d = new Date();
assert (!isNaN(d.valueOf()));

d = new Date("2015-01-01");
assert (d.valueOf() == 1420070400000);

d = new Date(1420070400000);
assert (d.valueOf() == 1420070400000);

d = new Date(2015,0,1,0,0,0,0);
assert (d.valueOf() - (d.getTimezoneOffset() * 60000) == 1420070400000);

d = new Date(8.64e+15);
assert (d.valueOf() == 8.64e+15);

d = new Date(8.64e+15 + 1);
assert (isNaN(d.valueOf()));

d = new Date(20000000, 0, 1);
assert (isNaN(d.valueOf()));

d = new Date(0, 20000000, 1);
assert (isNaN(d.valueOf()));

var Obj = function (val)
{
  this.value = val;
  this.valueOf = function () { throw new ReferenceError ("valueOf-" + this.value); };
  this.toString = function () { throw new ReferenceError ("toString-"+ this.value); };
};

try
{
  d = new Date (new Obj (1), new Obj (2));
  // Should not be reached.
  assert (false);
}
catch (e)
{
  assert (e instanceof ReferenceError);
  assert (e.message === "valueOf-1");
}

assert (typeof Date (2015) == "string");
assert (typeof Date() != typeof (new Date ()));
assert (Date (Number.NaN) == Date ());
