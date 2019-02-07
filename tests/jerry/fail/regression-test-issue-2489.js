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

Object.defineProperty(Object.prototype, 0, {'get': function() { throw $ }});
/**
 * The below line is added becase the patch #2526 introduces internal properties for promises.
 * The Reference Error this issue produced can still be reproduced by calling this line.
 * The reason it was present before is that Promise's 0th property was Promise which could be modified
 * with the above line, and the engine getting that property for internal purposes got the `throw $` instead.
 * Thanks to internal properties, it can't be modified anymore from JS side, therefore Promise won't trigger the error.
 * To keep this issue's output as it was before, the `Array.prototype[0];` line is added.
 */
Array.prototype[0];
Promise.all();
