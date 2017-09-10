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

/* Maximum 256 paramaters are allowed. */
var src = "(function (";
for (var i = 0; i < 254; i++)
    src += "a" + i + ", ";
src += "b) { var c = 1; })()";

eval(src);

/* More than 256 parameters is a syntax error. */
var src = "(function (";
for (var i = 0; i < 255; i++)
    src += "a" + i + ", ";
src += "b) { })()";

try {
  eval(src);
  assert(false);
} catch (e) {
  assert(e instanceof SyntaxError);
}

/* Maximum 256 local variables are stored in registers.
 * The rest is stored in the lexical environment. */
var src = "(function () {";
for (var i = 0; i < 400; i++)
    src += "var a" + i + " = 5; ";
src += "})()";

eval(src);
