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

var foo = function (c, y) {
};
var check = foo.hasOwnProperty("length") && foo.length === 2;

foo.length = 12;
if (foo.length === 12)
  check = false;

for (p in foo)
{
  if (p === "length")
    check = false;
}
delete foo.length;
if (!foo.hasOwnProperty("length"))
  check = false;

assert(check === true);
