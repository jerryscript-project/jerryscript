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

var d;

d = Date.UTC(undefined);
assert (isNaN(d));

d = Date.UTC({});
assert (isNaN(d));

d = Date.UTC(2015);
assert (isNaN(d));

d = Date.UTC(2000 + 15, 0);
assert (d == 1420070400000);

d = Date.UTC(2015, 0);
assert (d == 1420070400000);

d = Date.UTC(2015, 0, 1);
assert (d == 1420070400000);

d = Date.UTC(2015, 0, 1, 0);
assert (d == 1420070400000);

d = Date.UTC(2015, 0, 1, 0, 0);
assert (d == 1420070400000);

d = Date.UTC(2015, 0, 1, 0, 0, 0);
assert (d == 1420070400000);

d = Date.UTC(2015, 0, 1, 0, 0, 0, 0);
assert (d == 1420070400000);

d = Date.UTC(2015, 6, 3, 14, 35, 43, 123);
assert (d == 1435934143123);
