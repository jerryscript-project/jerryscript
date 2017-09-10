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

var wrongFormats = ["",
                    "2",
                    "20",
                    "201",
                    "2015-",
                    "2015-01-",
                    "2015-01-01-",
                    "qwerty",
                    "2015-01-01T",
                    "2015-01-01T1:1",
                    "2015-01-01T01",
                    "2015-01-01T01",
                    "2015-01-01T01:01F",
                    "T2015",
                    "2015-01-01Z",
                    "2015-01-01+01:00",
                    "2015-01-01T00:00+01",
                    "2015-01-01T00:00+1",
                    "2015-01-01T00:00-01",
                    "2015-01-01T00:00.000",
                    "2015-01-01T00:00:",
                    "2015-01-01T00:",
                    "2015-01-01T00:00:00.1",
                    "2015-01-01T00:00:00.01",
                    "2015-01-01T00:00+01:00Z",
                    "2015/01/01",
                    "2015-01-32",
                    "2015--1",
                    "2015-13",
                    "2015-01--1",
                    "-215",
                    "-215-01-01",
                    "2015-01-00",
                    "2015-01-01T25:00",
                    "2015-01-01T00:60",
                    "2015-01-01T-1:00",
                    "2015-01-01T00:-1",
                    "2e+3"];

for (i in wrongFormats) {
  var d = Date.parse(wrongFormats[i]);
  assert (isNaN(d));
}

var d;

d = Date.parse(undefined);
assert (isNaN(d));

d = Date.parse({});
assert (isNaN(d));

d = Date.parse(2000 + 15);
assert (d == 1420070400000);

d = Date.parse("2015");
assert (d == 1420070400000);

d = Date.parse("2015-01");
assert (d == 1420070400000);

d = Date.parse("2015-01-01");
assert (d == 1420070400000);

d = Date.parse("2015-01T00:00");
assert (d == 1420070400000);

d = Date.parse("2015-01T00:00:00");
assert (d == 1420070400000);

d = Date.parse("2015-01T00:00:00.000");
assert (d == 1420070400000);

d = Date.parse("2015-01T24:00:00.000");
assert (d == 1420070400000);

d = Date.parse("2015-01T00:00:00.000+03:00");
assert (d == 1420059600000);

d = Date.parse("2015-01T00:00:00.000-03:00");
assert (d == 1420081200000);

d = Date.parse("2015-07-03T14:35:43.123+01:30");
assert (d == 1435928743123);
