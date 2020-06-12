/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Check that Object.getOwnPropertyNames does NOT include Symbols by default.
var asd = Symbol ("asd");
var foo = Symbol ("foo");
var bar = Symbol ("bar");
var result = Object.getOwnPropertyNames ({1: 5, "a": 6, [foo]: 7, [asd]: 8, [bar]: 9});
assert (!Object.hasOwnProperty (result, foo));
assert (!Object.hasOwnProperty (result, asd));
assert (!Object.hasOwnProperty (result, bar));
