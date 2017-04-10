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

var a = new Promise(function(f, r){
  r(0);
});

a
.then(function f1(x) {
  return x + 1; // unreachable
}, function r1(x){
  return x + 10;
})
.then(function f2(x) {
  throw x + 100
})
.then(function f3(x) {
  return x + 1000 //unreachable
}, function r3(x) {
  return x + 10000
})
.then(function(x) {
  assert (x === 10110);
})
