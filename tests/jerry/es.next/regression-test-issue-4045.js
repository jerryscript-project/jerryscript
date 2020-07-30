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

var a = new Proxy(eval, {});
var b = {
    y : 2
};
b.__proto__ = a;

var f = [];
var c = [];
var d = new Date();
d.__proto__ = b;
a.__proto__ = f;

c.__proto__ = d;


var a1 = new String();
a1.__proto__ = c;

assert(a.__proto__ === f);
assert(b.__proto__.__proto__ === f);
assert(d.__proto__.__proto__ === a);
assert(c.__proto__.__proto__.__proto__ === a);
assert(a1.__proto__.__proto__.__proto__.__proto__ === a);

var e = []
a1.__proto__ = e;

assert(a1.__proto__ === e);
