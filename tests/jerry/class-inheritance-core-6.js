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

 class A extends Array {
   constructor (a, b, c, d, e) {
     eval ("eval ('super (a, b, c)')");
     this.a = 6;
     return new String ("foo");
   }

   f () {
     return 5;
   }
 }

 class B extends A {
   constructor (a, b, c, d) {
    eval ("eval ('super (a, b, c, d)')");
    assert (super.f () === 5);
   }
 }

 var a = new B (1, 2, 3, 4, 5, 6);
 assert (a.a === undefined);
 assert (a[0] + a[1] + a[2] === "foo");
