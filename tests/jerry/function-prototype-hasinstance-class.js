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

class base {
  constructor (value) {
    this.member = value;
  }

  method () {
    return this.member;
  }
}

class sub {
  constructor (value) {
    this.member = value;
  }
}

var obj_base = new base (3);
var obj_sub = new sub (4);

assert (base[Symbol.hasInstance](obj_base) === true);
assert (base[Symbol.hasInstance](obj_sub) === false);

assert (sub[Symbol.hasInstance](obj_base) === false);
assert (sub[Symbol.hasInstance](obj_sub) === true);


class sub_c extends base {
  constructor (value) {
    super(value);
    this.member = value;
  }
}

var obj_sub_c = new sub_c (5);

assert (base[Symbol.hasInstance](obj_sub_c) === true);

assert (sub_c[Symbol.hasInstance](obj_base) === false);
assert (sub_c[Symbol.hasInstance](obj_sub_c) === true);
