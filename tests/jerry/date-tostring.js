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

assert (new Date (NaN) == "Invalid Date");
assert (new Date (Infinity, 1, 1, 0, 0, 0) == "Invalid Date");
assert (new Date (2015, Infinity, 1, 0, 0, 0) == "Invalid Date");
assert (new Date (2015, 7, 1, 0, Infinity, 0) == "Invalid Date");
assert (new Date (NaN, 1, 1, 0, 0, 0) == "Invalid Date");
assert (new Date (2015, NaN, 1, 0, 0, 0) == "Invalid Date");
assert (new Date (2015, 7, 1, 0, NaN, 0) == "Invalid Date");
assert (/Fri Feb 13 2015 \d{2}:\d{2}:\d{2} GMT\+\d{2}:\d{2}/.test (new Date ("2015-02-13")));
assert (/Wed Jul 08 2015 \d{2}:\d{2}:\d{2} GMT\+\d{2}:\d{2}/.test (new Date ("2015-07-08T11:29:05.023")));

try
{
  Date.prototype.toString.call(-1);
  assert (false);
}
catch (e)
{
  assert (e instanceof TypeError);
}

var date = new Date(0);
assert (/Thu Jan 01 1970 \d{2}:\d{2}:\d{2} GMT\+\d{2}:\d{2}/.test (date.toString()));
assert (date.toUTCString() === "Thu, 01 Jan 1970 00:00:00 GMT");
assert (date.toISOString() === "1970-01-01T00:00:00.000Z");

date = new Date("2015-08-12T09:40:20.000Z")
assert (/Wed Aug 12 2015 \d{2}:\d{2}:\d{2} GMT\+\d{2}:\d{2}/.test (date.toString()));
assert (date.toUTCString() === "Wed, 12 Aug 2015 09:40:20 GMT");
assert (date.toISOString() === "2015-08-12T09:40:20.000Z");

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
}

assert (new Date (NaN).toTimeString () == "Invalid Date");
assert (new Date (Number.POSITIVE_INFINITY).toString () === "Invalid Date");
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
}

assert (new Date ("2015-07-16").toISOString () == "2015-07-16T00:00:00.000Z");
assert (new Date ("2015-07-16T11:29:05.023").toISOString () == "2015-07-16T11:29:05.023Z");

try
{
  new Date (NaN).toISOString ();
  assert (false);
}
catch (e)
{
  assert (e instanceof RangeError);
}

try
{
  new Date (Number.POSITIVE_INFINITY).toISOString ();
  assert (false);
}
catch (e)
{
  assert (e instanceof RangeError);
}

try
{
  Date.prototype.toISOString.call(-1);
  assert (false);
}
catch (e)
{
  assert (e instanceof TypeError);
}

assert (new Date (NaN).toUTCString () == "Invalid Date");
assert (new Date ("2015-07-16").toUTCString () == "Thu, 16 Jul 2015 00:00:00 GMT");
assert (new Date ("2015-07-16T11:29:05.023").toUTCString () == "Thu, 16 Jul 2015 11:29:05 GMT");

try
{
  Date.prototype.toUTCString.call(-1);
  assert (false);
}
catch (e)
{
  assert (e instanceof TypeError);
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
assert (new Date (date_time).toISOString () == "2015-07-08T11:29:05.023Z");

assert (typeof Date (2015) == "string");
assert (typeof Date() != typeof (new Date ()));
assert (Date () == (new Date ()).toString ());
assert (Date (2015, 1, 1) == (new Date ()).toString ());
assert (Date (Number.NaN) == Date ());

assert (new Date ("2015-07-08T11:29:05.023Z").toISOString() == "2015-07-08T11:29:05.023Z");

// corner cases
assert (new Date (-8640000000000001).toString() == "Invalid Date")
assert (new Date (-8640000000000000).toString() == "Tue Apr 20 -271821 00:00:00 GMT+00:00")

assert (new Date(-62167219200001).toString() == "Fri Dec 31 -0001 23:59:59 GMT+00:00")
assert (new Date(-62167219200000).toString() == "Sat Jan 01 0000 00:00:00 GMT+00:00")

assert (new Date(-61851600000001).toString() == "Thu Dec 31 0009 23:59:59 GMT+00:00")
assert (new Date(-61851600000000).toString() == "Fri Jan 01 0010 00:00:00 GMT+00:00")

assert (new Date(-59011459200001).toString() == "Thu Dec 31 0099 23:59:59 GMT+00:00")
assert (new Date(-59011459200000).toString() == "Fri Jan 01 0100 00:00:00 GMT+00:00")

assert (new Date(-30610224000001).toString() == "Tue Dec 31 0999 23:59:59 GMT+00:00")
assert (new Date(-30610224000000).toString() == "Wed Jan 01 1000 00:00:00 GMT+00:00")

assert (new Date(-1).toString() == "Wed Dec 31 1969 23:59:59 GMT+00:00")
assert (new Date(0).toString() == "Thu Jan 01 1970 00:00:00 GMT+00:00")
assert (new Date(1).toString() == "Thu Jan 01 1970 00:00:00 GMT+00:00")

assert (new Date(253402300799999).toString() == "Fri Dec 31 9999 23:59:59 GMT+00:00")
assert (new Date(253402300800000).toString() == "Sat Jan 01 10000 00:00:00 GMT+00:00")

assert (new Date (8640000000000000).toString() == "Sat Sep 13 275760 00:00:00 GMT+00:00")
assert (new Date (8640000000000001).toString() == "Invalid Date")
