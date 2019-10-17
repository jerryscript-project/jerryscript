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

function base (value) {
  this.member = value;
}

base.prototype.method = function () { return this.member; }

function sub (value) {
  this.member = value;
}

sub.prototype = base;


var obj_base = new base (3);
var obj_sub = new sub (4);

assert (base[Symbol.hasInstance] (obj_base) === true);
assert (base[Symbol.hasInstance] (obj_sub) === false);
assert (Object[Symbol.hasInstance] (obj_base) === true);
assert (Object[Symbol.hasInstance] (obj_sub) === true);
assert (obj_base.method () === 3);

assert (sub[Symbol.hasInstance] (obj_base) === false);
assert (sub[Symbol.hasInstance] (obj_sub) === true);
assert (obj_sub.method === undefined);

function sub_c (value) {
  this.member = value;
}

sub_c.prototype = Object.create (base.prototype)
sub_c.prototype.constructor = sub_c

var obj_sub_c = new sub_c (5);

assert (base[Symbol.hasInstance] (obj_sub_c) === true);

assert (sub_c[Symbol.hasInstance] (obj_base) === false);
assert (sub_c[Symbol.hasInstance] (obj_sub_c) === true);
assert (Object[Symbol.hasInstance] (obj_sub_c) === true);
assert (Function.prototype[Symbol.hasInstance].call (sub_c, obj_sub_c) === true);

assert (obj_sub_c.method () === 5);

assert (base[Symbol.hasInstance] (3) === false);
assert (Number[Symbol.hasInstance] (33) === false);
assert (Number[Symbol.hasInstance] (new Number (33)) === true);
assert (Object[Symbol.hasInstance] (44) === false);
assert (Object[Symbol.hasInstance] (new Number (22)) === true);

assert (base[Symbol.hasInstance] ('demo') === false);
assert (String[Symbol.hasInstance] ('demo') === false);
assert (String[Symbol.hasInstance] (new String ('demo')) === true);
assert (Object[Symbol.hasInstance] ('demo') === false);
assert (Object[Symbol.hasInstance] (new String ('demo')) === true);

assert (base[Symbol.hasInstance] ([]) === false);
assert (base[Symbol.hasInstance] ([1, 2]) === false);
assert (Array[Symbol.hasInstance] ([1, 2]) === true);
assert (Array[Symbol.hasInstance] (new Array(1, 2)) === true);
assert (Object[Symbol.hasInstance] ([]) === true);
assert (Object[Symbol.hasInstance] (new Array()) === true);

assert (base[Symbol.hasInstance] (new RegExp('abc')) === false);
assert (RegExp[Symbol.hasInstance] (/abc/) === true);
assert (RegExp[Symbol.hasInstance] (new RegExp('abc')) === true);
assert (Object[Symbol.hasInstance] (/abc/) === true);
