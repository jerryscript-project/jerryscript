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

/* Prototype of NativeErrors should be Error */
assert(Object.getPrototypeOf(EvalError) === Error);
assert(Object.getPrototypeOf(RangeError) === Error);
assert(Object.getPrototypeOf(ReferenceError) === Error);
assert(Object.getPrototypeOf(SyntaxError) === Error);
assert(Object.getPrototypeOf(TypeError) === Error);
assert(Object.getPrototypeOf(URIError) === Error);
assert(Object.getPrototypeOf(AggregateError) === Error);
