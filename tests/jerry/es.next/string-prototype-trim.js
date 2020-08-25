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

var test = "  asd  ";
assert(test.trimStart() === "asd  ")
assert(test.trimStart().length === 5)
assert(test.trimLeft() === "asd  ")
assert(test.trimLeft().length === 5)
assert(String.prototype.trimStart === String.prototype.trimLeft)

assert(test.trimEnd() === "  asd")
assert(test.trimEnd().length === 5)
assert(test.trimRight() === "  asd")
assert(test.trimRight().length === 5)
assert(String.prototype.trimEnd === String.prototype.trimRight)

assert(test.trim() === "asd")
assert(test.trim().length === 3)
