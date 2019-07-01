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

var c = 0;
var p1 = this[c++];
assert (p1 === undefined);
assert (c === 1);

var p2 = this[c--];
assert (p2 === undefined);
assert (c === 0);

var p3 = this[++c];
assert (p3 === undefined);
assert (c === 1);

var p4 = this[--c];
assert (p4 === undefined);
assert (c === 0);
