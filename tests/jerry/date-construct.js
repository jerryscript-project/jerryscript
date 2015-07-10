// Copyright 2015 Samsung Electronics Co., Ltd.
// Copyright 2015 University of Szeged.
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

d = Date("abcd");
assert (isNaN(d.valueOf()));

d = Date();
assert (!isNaN(d.valueOf()));

d = Date("2015-01-01");
assert (d.valueOf() == 1420070400000);

d = Date(1420070400000);
assert (d.valueOf() == 1420070400000);

d = Date(2015,0,1,0,0,0,0);
assert (d.valueOf() == 1420070400000);

d = new Date();
assert (!isNaN(d.valueOf()));

d = new Date("2015-01-01");
assert (d.valueOf() == 1420070400000);

d = new Date(1420070400000);
assert (d.valueOf() == 1420070400000);

d = new Date(2015,0,1,0,0,0,0);
assert (d.valueOf() == 1420070400000);
