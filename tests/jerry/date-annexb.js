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

var d = new Date(1999, 1, 1);
assert (d.getYear() === 99);
d = new Date(1874, 4, 9);
assert (d.getYear() === -26);
d = new Date(2015, 8, 17);
assert (d.getYear() === 115);
d = new Date(NaN);
assert (isNaN (d.getYear()));

var d = new Date();
d.setYear(91);
assert (d.getFullYear() === 1991 && d.getYear() === 91);

d = new Date();
d.setYear(NaN);
assert (isNaN(d.valueOf()));

d = new Date();
d.setYear(2015);
assert (d.getFullYear() === 2015);

d = new Date(2000, 1, 29);
d.setYear(2004);
assert (d.getFullYear() === 2004 && d.getMonth() === 1 && d.getDate() === 29);
d.setYear(2015);
assert (d.getFullYear() === 2015 && d.getMonth() === 2 && d.getDate() === 1);

assert (/Thu, 17 Sep 2015 \d{2}:\d{2}:\d{2} GMT/.test (new Date("2015-09-17").toGMTString()));

d = new Date(NaN);
assert (d.toGMTString() === "Invalid Date");
