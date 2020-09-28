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

try {
  {
    A;
    class A { }
  }
  assert(false)
} catch (e) {
  assert(e instanceof ReferenceError)
}

try {
  {
    class A { [A] () {} }
  }
  assert(false)
} catch (e) {
  assert(e instanceof ReferenceError)
}

try {
  {
    var a = class A { [A] () {} }
  }
  assert(false)
} catch (e) {
  assert(e instanceof ReferenceError)
}

{
  class C {
    a = C
    static b = C
  }

  var X = C
  C = 6
  var c = new X

  assert(X.b === X)
  assert(c.a === X)
}

{
  let a = 6
  let b = 7
  class C {
    p = a + b
  }
  assert((new C).p === 13)
}

try {
  {
    class C { static a = C = 5  }
  }
  assert(false)
} catch (e) {
  assert(e instanceof TypeError)
}

try {
  {
    class C { static [C = 5] = 6 }
  }
  assert(false)
} catch (e) {
  assert(e instanceof ReferenceError)
}
