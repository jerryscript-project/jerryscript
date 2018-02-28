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

function parse (txt) {
  try {
    eval (txt)
    assert (false)
  } catch (e) {
    assert (e instanceof SyntaxError)
  }
}

var obj = {a: 1, b: 2, c:3, d:4};

var forIn =
  "for var prop in obj" +
  "   obj [prop] += 4"
parse (forIn)

var forIn =
  "for [var prop in obj]" +
  "   obj[prop] += 4;"
parse (forIn)

var forIn =
  "for (var prop obj)" +
  "   obj[prop] += 4;"
parse (forIn)

var forIn =
  "foreach (var prop in obj)" +
  "   obj[prop] += 4;"
parse (forIn)
