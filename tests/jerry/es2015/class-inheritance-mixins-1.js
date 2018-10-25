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

var calculatorMixin = Base => class extends Base {
  f () {
    return 1;
  }
};

var randomizerMixin = Base => class extends Base {
  g () {
    return 2;
  }
};

class A {
  constructor () { }
}

class B extends calculatorMixin (randomizerMixin (A)) {

}

var b = new B ();
assert (b.f () === 1)
assert (b.g () === 2);
