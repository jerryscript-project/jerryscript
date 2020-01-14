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
  [...RegExp.$.$] = String();
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

var o = { a : { b : { c : { d: { e : { } } } } } };

[...o.a.b.c.d.e] = "foo";

assert(o.a.b.c.d.e.length === 3);
assert(o.a.b.c.d.e[0] === "f");
assert(o.a.b.c.d.e[1] === "o");
assert(o.a.b.c.d.e[2] === "o");

[o.a.b.c.d.e] = "foo";

assert(o.a.b.c.d.e === "f");

[this.o.a.b.c.d.e] = "bar";
assert(this.o.a.b.c.d.e === "b");

[...this.o.a.b.c.d.e] = "bar";
assert(this.o.a.b.c.d.e.length === 3);
assert(this.o.a.b.c.d.e[0] === "b");
assert(this.o.a.b.c.d.e[1] === "a");
assert(this.o.a.b.c.d.e[2] === "r");
