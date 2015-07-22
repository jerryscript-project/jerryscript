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

assert ([].toLocaleString() === "");
assert ([1].toLocaleString() === "1");
assert ([1,2].toLocaleString() === "1,2");
assert ([1,2,3].toLocaleString() === "1,2,3");

var test_ok = {
  length: 1,
  toLocaleString: function() { return "1"; }
};

assert ([3, test_ok, 4, test_ok].toLocaleString() === "3,1,4,1");


var obj = { toLocaleString: function() {} };
var test_non_str_locale = [undefined, obj, null, obj, obj];

assert(test_non_str_locale.toLocaleString() === ",undefined,,undefined,undefined");

var test_fail = {
  toLocaleString: "FAIL"
};

try {
  [test_fail].toLocaleString();
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}


var test_fail_call = {
  toLocaleString: function() { throw new ReferenceError("foo"); }
};


try {
  [1, 2, test_fail_call].toLocaleString();
  assert (false);
} catch (e) {
  assert (e.message === "foo");
  assert (e instanceof ReferenceError);
}
