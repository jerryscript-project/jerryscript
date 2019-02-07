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

 function isInstanceofTypedArray (instance) {
   assert (instance instanceof C);
   assert (instance instanceof B);
   assert (instance instanceof A);
   assert (instance instanceof Uint8Array);
 }

 class A extends Uint8Array {
   f () {
     return 5;
   }
 }

 class B extends A {
   g () {
     return super.f ();
   }
 }

 class C extends B {
   h () {
     return super.g ();
   }
 }

 var c = new C ([1, 2, 3, 4, 5, 6]);

 isInstanceofTypedArray (c);
 assert (c.length === 6);
 assert (c.f () === 5)
 assert (c.g () === 5)
 assert (c.h () === 5)

/* TODO: Enable these tests after Symbol has been implemented
 var mapped = c.map ((x) => x * 2);
 isInstanceofTypedArray (mapped);

 for (var i = 0; i < mapped.length; i++) {
   assert (mapped[i] == c[i] * 2);
 }

 var filtered = c.filter ((x) => x > 100);
 isInstanceofTypedArray (filtered);
 assert (filtered.length === 0);

 c.constructor = 5;

 try {
   mapped = c.map ((x) => x * 2);
   assert (false);
 } catch (e) {
   assert (e instanceof TypeError)
 }

 try {
   filtered = c.filter ((x) => x > 100);
   assert (false);
 } catch (e) {
   assert (e instanceof TypeError)
 }
*/
