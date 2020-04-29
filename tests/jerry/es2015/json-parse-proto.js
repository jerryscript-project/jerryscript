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

/* JSON.parse should treat __proto__ as regular property name */

var str1 = '{"__proto__": [] }';
var obj1 = JSON.parse(str1);
assert(Object.getPrototypeOf(obj1) === Object.prototype);
assert(Array.isArray(obj1.__proto__));
