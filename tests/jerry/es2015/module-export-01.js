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

export {};
export {a as aa,};
export {b as bb, c as cc};
export {d};
export var x = 42;
export function f(a) {return a;};
export class Dog {
  constructor (name) {
    this.name = name;
  }

  speak() {
    return this.name + " barks."
  }
};
export default "default";

var a = "a";
var b = 5;
var c = function(a) { return 2 * a;}
var d = [1,2,3];

assert (x === 42);
assert (f(1) === 1);
var dog = new Dog("Pluto")
assert(dog.speak() === "Pluto barks.")
