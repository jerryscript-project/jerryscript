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

class Simple {
  constructor () {
    assert (new.target === Simple);
    this.called = 0;
  }

  get getter () {
    this.called++;
    return new.target;
  }

  set setter (val) {
    assert (new.target === undefined);
    this.called++;
  }
}

var smp = new Simple ();
assert (smp.getter === undefined);
assert (smp.called === 1);
smp.setter = -1;
assert (smp.called === 2);

class BaseA {
  constructor () {
    assert (new.target === SubA);
  }
}

class SubA extends BaseA {
  constructor () {
    super ();
    assert (new.target === SubA);
  }
}

new SubA ();

/* TODO: enable if there is class name support. */
/*
class Named {
  constructor () {
    assert(new.target.name == "Named");
  }
}

new Named ();
*/
