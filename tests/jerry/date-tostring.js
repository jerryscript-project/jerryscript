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

assert (new Date (NaN) == "Invalid Date");
assert (new Date ("2015-02-13") == "2015-02-13T00:00:00.000");
assert (new Date ("2015-07-08T11:29:05.023") == "2015-07-08T11:29:05.023");

try
{
  Date.prototype.toString.call(-1);
  assert (false);
}
catch (e)
{
  assert (e instanceof TypeError);
  assert (e.message === "Incompatible type");
}

assert (new Date (NaN).toDateString () == "Invalid Date");
assert (new Date ("2015-02-13").toDateString () == "2015-02-13");
assert (new Date ("2015-07-08T11:29:05.023").toDateString () == "2015-07-08");

try
{
  Date.prototype.toDateString.call(-1);
  assert (false);
}
catch (e)
{
  assert (e instanceof TypeError);
  assert (e.message === "Incompatible type");
}

assert (new Date (NaN).toTimeString () == "Invalid Date");
assert (Date (Number.POSITIVE_INFINITY).toString () === "Invalid Date");
assert (new Date ("2015-02-13").toTimeString () == "00:00:00.000");
assert (new Date ("2015-07-08T11:29:05.023").toTimeString () == "11:29:05.023");

try
{
  Date.prototype.toTimeString.call(-1);
  assert (false);
}
catch (e)
{
  assert (e instanceof TypeError);
  assert (e.message === "Incompatible type");
}

assert (new Date (NaN).toISOString () == "Invalid Date");
assert (new Date ("2015-07-16").toISOString () == "2015-07-16T00:00:00.000Z");
assert (new Date ("2015-07-16T11:29:05.023").toISOString () == "2015-07-16T11:29:05.023Z");

try
{
  Date.prototype.toISOString.call(-1);
  assert (false);
}
catch (e)
{
  assert (e instanceof TypeError);
  assert (e.message === "Incompatible type");
}

assert (new Date (NaN).toUTCString () == "Invalid Date");
assert (new Date ("2015-07-16").toUTCString () == "2015-07-16T00:00:00.000Z");
assert (new Date ("2015-07-16T11:29:05.023").toUTCString () == "2015-07-16T11:29:05.023Z");

try
{
  Date.prototype.toUTCString.call(-1);
  assert (false);
}
catch (e)
{
  assert (e instanceof TypeError);
  assert (e.message === "Incompatible type");
}

assert (new Date (NaN).toJSON () == null);
assert (new Date ("2015-07-16").toJSON () == "2015-07-16T00:00:00.000Z");
assert (new Date ("2015-07-16T11:29:05.023").toJSON () == "2015-07-16T11:29:05.023Z");

try
{
  Date.prototype.toJSON.call(-1);
  assert (false);
}
catch (e)
{
  assert (e instanceof TypeError);
}

date_time = new Date ("2015-07-08T11:29:05.023").toJSON ();
assert (new Date (date_time) == "2015-07-08T11:29:05.023");
