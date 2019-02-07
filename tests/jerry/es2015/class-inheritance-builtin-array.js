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

 function isInstanceofArray (instance) {
   assert (instance instanceof C);
   assert (instance instanceof B);
   assert (instance instanceof A);
   assert (instance instanceof Array);
 }

 class A extends Array {
   f () {
     return 5;
   }
 }

 class B extends A {
   g () {
     return eval ("eval ('super.f ()')");
   }
 }

 class C extends B {
   h () {
     return eval ('super.g ()');
   }
 }

 var c = new C (1, 2, 3, 4, 5, 6);

 isInstanceofArray (c);
 c.push (7);
 assert (c.length === 7);
 assert (c.f () === 5);
 assert (c.g () === 5);
 assert (c.h () === 5);

/* TODO: Enable these tests after Symbol has been implemented
 // Test built-in Array prototype methods
 var mapped = c.map ((x) => x * 2);
 isInstanceofArray (mapped);

 for (var i = 0; i < mapped.length; i++) {
   assert (mapped[i] == c[i] * 2);
 }

 var concated = c.concat (c);
 isInstanceofArray (concated);

 for (var i = 0; i < concated.length; i++) {
   assert (concated[i] == c[i % (concated.length / 2)]);
 }

 var sliced = c.slice (c);
 isInstanceofArray (sliced);

 for (var i = 0; i < sliced.length; i++) {
   assert (sliced[i] == c[i]);
 }

 var filtered = c.filter ((x) => x > 100);
 isInstanceofArray (sliced);
 assert (filtered.length === 0);

 var spliced = c.splice (c.length - 1);
 isInstanceofArray (spliced);
 assert (spliced.length === 1);
 assert (spliced[0] === 7);

 c.constructor = 5;

 try {
   mapped = c.map ((x) => x * 2);
   assert (false);
 } catch (e) {
   assert (e instanceof TypeError);
 }

 try {
   concated = c.concat (c);
   assert (false);
 } catch (e) {
   assert (e instanceof TypeError);
 }

 try {
   sliced = c.slice (c);
   assert (false);
 } catch (e) {
   assert (e instanceof TypeError);
 }

 try {
   filtered = c.filter ((x) => x > 100);
   assert (false);
 } catch (e) {
   assert (e instanceof TypeError);
 }

 try {
   spliced = c.splice (0);
   assert (false);
 } catch (e) {
   assert (e instanceof TypeError);
 }
*/
