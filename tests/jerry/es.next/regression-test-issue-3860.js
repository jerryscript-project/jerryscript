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

try {
  Object (...1, {});
  assert (false);
} catch (ex) {
  // expected error: "TypeError: object is not iterable"
  assert (ex instanceof TypeError);
}

try {
  Object (...1, {}, {});
  assert (false);
} catch (ex) {
  // expected error: "TypeError: object is not iterable"
  assert (ex instanceof TypeError);
}

try {
  Object (...1, { "prop": 2 }, 1, { "prop": 2 });
  assert (false);
} catch (ex) {
  // expected error: "TypeError: object is not iterable"
  assert (ex instanceof TypeError);
}

try {
  Object (...1, "str");
  assert (false);
} catch (ex) {
  // expected error: "TypeError: object is not iterable"
  assert (ex instanceof TypeError);
}

try {
  Object (...[], { "prop": 2 }, 1, { "prop": 2 }, ...1);
  assert (false);
} catch (ex) {
  // expected error: "TypeError: object is not iterable"
  assert (ex instanceof TypeError);
}
