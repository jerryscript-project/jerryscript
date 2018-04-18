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

var a = 1;

function f() {
  throw ++a;
}

function g() {
  f();
}

function h() {
  g();
}

try {
  h();
} catch (e) {
  ++a;
}

try {
  h();
} catch (e) {
  ++a;

  try {
    throw "Catch again";
  } catch (e) {
    ++a;
  }
}

try {
  try {
    h();
  } finally {
    ++a;
  }
  ++a; /* Should not happen. */
} catch (e) {
  ++a;
}

try {
  try {
    h();
  } finally {
    ++a;
    throw "Replace the other error";
  }
} catch (e) {
  ++a;
}

try {
  break_try_label: try {
    h();
  } finally {
    ++a;
    break break_try_label;
  }
  throw "Should be caught";
} catch (e) {
  ++a;
}

h();
