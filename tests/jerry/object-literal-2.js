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

var a = {get a(){return undefined}, set a(b){}}

var b = {if:0, else:1, try:2, catch:3, finally:4, let:5}

assert (b.if + b.else + b.try + b.catch + b.finally + b.let === 15)

function c() {
  "use strict"
  var b = {let:15, enum:10}
  assert (b.let + b.enum === 25)
}
c();

function d () {
  "use strict";

  try {
    /* 'let' is a FutureReservedWord in strict mode code */
    eval ('var a = { get prop () { let = 1; } }');
    assert (false);
  } catch (e) {
    assert (e instanceof SyntaxError);
  }
}
d ();
