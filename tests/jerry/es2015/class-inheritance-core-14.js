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

class A {
  constructor () {
    this.a = 5;
  }

  f () {
    return 10;
  }

  super () {
    this.super = 10;
    return 15;
  }
}

class B extends A {
  constructor () {
    super ();
    assert (super.f === A.prototype.f);
    super.f = 8;
    assert (this.f === 8);
    assert (super.f === A.prototype.f);

    assert (this.a === 5);
    super.a = 10;
    assert (this.a === 10);

    assert (super.super () === 15);
    assert (this.super === 10);
    super.super = 20;
    assert (this.super === 20);
    assert (super.super () === 15);
  }
}

var b = new B;
assert (b.f === 8);
assert (b.a === 10);
