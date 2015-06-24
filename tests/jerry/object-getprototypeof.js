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

try {
  var v;
  Object.getPrototypeOf(v);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  Object.getPrototypeOf("foo");
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  Object.getPrototypeOf(60);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  var y = Object.getPrototypeOf(null);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

var obj = { x : "foo" };
assert (Object.getPrototypeOf(obj) === Object.prototype);

var constructor = function () {};
constructor.prototype = obj;

var d_obj = new constructor();
assert (Object.getPrototypeOf(d_obj) === obj);

obj = Object.create(null);
assert (Object.getPrototypeOf(obj) === null);
