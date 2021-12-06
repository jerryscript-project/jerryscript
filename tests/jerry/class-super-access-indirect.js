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

class Base {
  constructor () {
    this.parent_value = 100;
  }

  parent_value () {
    return this.parent_value;
  }

  parent_value_arg (a, b, c) {
    if (c) {
      return this.parent_value + a + b + c;
    } else if (b) {
      return this.parent_value + a + b;
    } else {
      return this.parent_value + a;
    }
  }

  method () {
    return {
      method: function (a, b, c, d, e) { return -50 + a + b + c + d + e; }
    }
  }
}

class Target extends Base {
  constructor () {
    super ();
    this.parent_value = -10;
  }

  parent_value () {
    throw new Error ('(parent_value)');
  }

  parent_value_indirect () {
    return super.parent_value.call (this);
  }

  parent_value_indirect_arg (a, b, c) {
    if (c) {
      return super.parent_value_arg.call (this, a, b, c);
    } else if (b) {
      return super.parent_value_arg.call (this, a, b);
    } else {
      return super.parent_value_arg.call (this, a);
    }
  }

  method () {
    throw new Error ("(method)");
  }

  parent_method_dot () {
    return super.method.call (this).method (1, 2, 3, 4, 5)
  }

  parent_method_index() {
    return super['method'].call (this)['method'] (1, 2, 3, 4, 5);
  }
}


var obj = new Target();

assert (obj.parent_value_indirect () === -10);
assert (obj.parent_value_indirect_arg (1) === -9);
assert (obj.parent_value_indirect_arg (1, 2) === -7);
assert (obj.parent_value_indirect_arg (1, 2, 3) === -4);

try {
  obj.parent_value ();
  assert (false);
} catch (ex) {
  /* 'obj.parent_value is a number! */
  assert (ex instanceof TypeError);
}

assert (obj.parent_method_dot () === -35);
assert (obj.parent_method_index () === -35);

try {
  obj.method();
  assert (false);
} catch (ex) {
  assert (ex.message === '(method)');
}

var demo_object = {
  parent_value: 1000,
  method: function () {
    throw new Error ('Very bad!');
  }
}

assert (obj.parent_value_indirect_arg.call (demo_object, 1) === 1001);
assert (obj.parent_value_indirect_arg.call (demo_object, 1, 2) === 1003);

assert (obj.parent_method_dot.call (demo_object) === -35);
