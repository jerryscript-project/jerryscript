// Copyright JS Foundation and other contributors, http://js.foundation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

class Animal {}
class Dog extends Animal {
  static explain () {
    super.f = 8
  }
}

assert (Dog.f === undefined);
assert (Animal.f === undefined);

Dog.explain ()

assert (Animal.f === undefined);
assert (Dog.f === 8);
