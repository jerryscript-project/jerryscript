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

/* argument is not reference */
assert (delete 0 === true);
assert (delete "0" === true);
assert (delete (a = 1) === true);
assert (delete delete a === true);

/* argument is unresolvable reference */
assert (delete undefined_variable === true);

/* argument is object-based reference */
var a = [1];
assert (a[0] === 1);
assert (delete a[0] === true);
assert (a[0] == undefined);

var b = {c:0};
assert (b.c === 0);
assert (delete b.c === true);
assert (b.c === undefined);

/* argument is lexical environment-based reference */
var a = 1;
assert (a === 1);
assert (delete a === false);
assert (a === 1);
