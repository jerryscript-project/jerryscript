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

function Function_A() { }
function Function_B() { }
Function_B.prototype = new Function_A();

function Function_C() { }
Function_C.prototype = new Function_B();

function Function_D() { }
Function_D.prototype = new Function_C();

var d_instance = new Function_D();

assert (Function_A.prototype.isPrototypeOf (d_instance) === true)
assert (Function_A.prototype.isPrototypeOf (Array) === false)


assert (Function_A.prototype.isPrototypeOf.call(0, 0) === false);
assert (Function_A.prototype.isPrototypeOf.call(Function_A, 0) === false);

assert (Function.prototype.isPrototypeOf (Object) === true)
