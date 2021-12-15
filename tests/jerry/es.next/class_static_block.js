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

function check_syntax_error(code) {
  try {
    eval(code)
    assert(false)
  } catch (e) {
    assert(e instanceof SyntaxError)
  }
}

check_syntax_error("static {}");
check_syntax_error("class C { #static {} }");
check_syntax_error("function foo() { static {} }");
check_syntax_error("class C { static { return; } }");
check_syntax_error("class C { static { break; } }");
check_syntax_error("class C { static { continue; } }");
check_syntax_error("class C { static { await => 0; } }");
check_syntax_error("class C { static { yield } }");
check_syntax_error("class C { static { super(); } }");
check_syntax_error("class C { static { let a; let a; } }");
check_syntax_error("class C { static { label: label: 0 } }");
check_syntax_error("class C { static { let #a; } }");
check_syntax_error("class C { static { (class { [arguments]() { } }); } }");
check_syntax_error("class C { static {x: while (false) {continue y; } } }");
check_syntax_error("function* g() { class C { static { yield; } } }");

try {
  class C {
    static {
      function foo() {
        return this.a
      };

      foo()
    }
  }
  assert(false)
} catch (e) {
  assert(e instanceof TypeError)
}

class Parent {
  static fieldP = 'fieldP'
}

class C1 extends Parent{
  static a = 0;
  static #privateField = 1;

  static {
    assert(this.a === 0)
    assert(this.#privateField === 1)

    this.a = 1
    this.b = super.fieldP
    this.c = new.target

    assert(this.a === 1)
    assert(this.b === 'fieldP')
    assert(this.c === undefined)
    assert(this.d === undefined)
  }

  static d = 2

  static {
    assert(this.b === 'fieldP')
    assert(this.d === 2)
  }

  static {
    function f() {
      assert(this === undefined)
    }

    let a = 11
    assert (this.a !== a)
  }

}

class C2 {
  getPrivateField;
  static getPrivateField2;
  static invalidAccess;
}

let getPrivateField;

class C3 {
  #privateField = 'invalid';
  static #staticPrivateField = 'private';

  constructor(p) {
    this.#privateField = p;
  }

  static {
    getPrivateField = (d) => d.#privateField;
    C2.getPrivateField = () => this.#staticPrivateField;
    C2.getPrivateField2 = () => this.#staticPrivateField;
    C2.invalidAccess = () => this.#privateField;
  }
}

assert(getPrivateField(new C3('private field')) == 'private field');
assert(C2.getPrivateField() === 'private')
assert(C2.getPrivateField2() === 'private')

try {
  C2.invalidAccess()
  assert(false)
} catch (e) {
  assert(e instanceof TypeError)
}

let a = 'outter'
let b, c

class C5 {
  static {
    let a = 'first block'
    b = a
  }

  static {
    let a = "second block"
    c = a
  }
}

assert(a === "outter")
assert(b === "first block")
assert(c === "second block")
