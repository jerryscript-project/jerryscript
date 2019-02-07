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

 order = 0;

 var Mixin1 = (superclass) => class extends superclass {
   foo () {
     assert (order++ == 1)
     if (super.foo) {
       super.foo ();
     }
   }
 };

 var Mixin2 = (superclass) => class extends superclass {
   foo () {
     assert (order++ == 2)
     if (super.foo) {
       assert (super.foo () === 5);
     }
   }
 };

 class S {
   foo () {
     assert (order++ == 3)
     return 5;
   }
 }

 class C extends Mixin1 (Mixin2 (S)) {
   foo () {
     assert (order++ == 0)
     super.foo ();
   }
 }

 new C ().foo ()
