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

var expr = "Dummy value";

var f = function expr() {
  assert(expr === f);
  expr = 6;
  assert(expr === f);
  assert(!(delete expr));
  assert(expr === f);
}

f();

f = function expr() {
  assert(expr === undefined);
  var expr = 6;
  assert(expr === 6);
}

f();

var f = function expr() {
  assert(expr === f);
  eval("var expr = 8");
  assert(expr === 8);
}

f();

var f = function expr(i) {
  assert(expr === f);

  if (i > 0) {
    expr(i - 1);
  }
}

f(10);
