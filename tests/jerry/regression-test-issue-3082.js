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

var obj = {
  a: "str",
  b: function() {},
  c: true
}

assert (JSON.stringify (obj) === "{\"a\":\"str\",\"c\":true}");
assert (JSON.stringify (obj, null, 2) === "{\n  \"a\": \"str\",\n  \"c\": true\n}");

var obj = {
  f: function() {}
}

assert (JSON.stringify (obj) === "{}");
assert (JSON.stringify (obj, null, 2) === "{}");

var obj = {
  f: function() {},
  a: null
}

assert (JSON.stringify (obj) === "{\"a\":null}");
assert (JSON.stringify (obj, null, 2) === "{\n  \"a\": null\n}");

var obj = {
  a: false,
  f: function() {}
}

assert (JSON.stringify (obj) === "{\"a\":false}");
assert (JSON.stringify (obj, null, 2) === "{\n  \"a\": false\n}");
