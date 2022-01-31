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

(function tc_19_01_02__003() {
  function test_set_prototype_of_success(o, proto, msg)
  {
    assert(o === Object.setPrototypeOf(o, proto));

    if (msg)
    {
      print(msg + " PASS");
    }
  }

  (function test_nonobject_o(undefined)
  {
    test_set_prototype_of_success(true, new Object(), "Object.setPrototypeOf(boolean, ...)");
    test_set_prototype_of_success(3.14, new Object(), "Object.setPrototypeOf(number, ...)");
    test_set_prototype_of_success("xyz", new Object(), "Object.setPrototypeOf(string, ...)");
  })()
})();

(function tc_19_01_02__006() {
  function test_set_prototype_of_success_set(o, proto, msg)
  {
    assert(o === Object.setPrototypeOf(o, proto));
    assert(proto === Object.getPrototypeOf(o));

    if (msg)
    {
      print(msg + " PASS");
    }
  }

  (function test_set_prototype_of(undefined)
  {
    test_set_prototype_of_success_set(new Object(), new Object(), "Object.setPrototypeOf(o1, o2)");
    test_set_prototype_of_success_set(new Object(), null, "Object.setPrototypeOf(o, null)");
  })()
})();

(function tc_19_01_02__004() {
  function test_set_prototype_of_error(o, proto, msg)
  {
    var name = "";

    try
    {
      Object.setPrototypeOf(o, proto);
    }
    catch (e)
    {
      name = e.name;
    }

    assert(name === "TypeError");

    if (msg)
    {
      print(msg + " PASS (XFAIL)");
    }
  }

  function test_set_prototype_of_success_set(o, proto, msg)
  {
    assert(o === Object.setPrototypeOf(o, proto));
    assert(proto === Object.getPrototypeOf(o));

    if (msg)
    {
      print(msg + " PASS");
    }
  }

  (function test_nonextensible_o(undefined)
  {
    var o = new Object();
    var o_proto = Object.getPrototypeOf(o);
    Object.preventExtensions(o);

    test_set_prototype_of_success_set(o, o_proto, "Object.setPrototypeOf(o_nonext, o_nonext.__proto__)");
    test_set_prototype_of_error(o, new Object(), "Object.setPrototypeOf(o_nonext, ...)");
  })()
})();

(function tc_19_01_02__002() {
  function test_set_prototype_of_error(o, proto, msg)
  {
    var name = "";

    try
    {
      Object.setPrototypeOf(o, proto);
    }
    catch (e)
    {
      name = e.name;
    }

    assert(name === "TypeError");

    if (msg)
    {
      print(msg + " PASS (XFAIL)");
    }
  }

  (function test_nonobject_proto(undefined)
  {
    test_set_prototype_of_error(new Object(), undefined, "Object.setPrototypeOf(..., undefined)");
    test_set_prototype_of_error(new Object(), true, "Object.setPrototypeOf(..., boolean)");
    test_set_prototype_of_error(new Object(), 3.14, "Object.setPrototypeOf(..., number)");
    test_set_prototype_of_error(new Object(), "xyz", "Object.setPrototypeOf(..., string)");
  })()
})();

(function tc_19_01_02__005() {
  function test_set_prototype_of_error(o, proto, msg)
  {
    var name = "";

    try
    {
      Object.setPrototypeOf(o, proto);
    }
    catch (e)
    {
      name = e.name;
    }

    assert(name === "TypeError");

    if (msg)
    {
      print(msg + " PASS (XFAIL)");
    }
  }

  (function test_circularity(undefined)
  {
    var o = new Object();

    test_set_prototype_of_error(o, o, "Object.setPrototypeOf(o, o)");
  })()
})();

(function tc_19_01_02__001() {
  function test_set_prototype_of_error(o, proto, msg)
  {
    var name = "";

    try
    {
      Object.setPrototypeOf(o, proto);
    }
    catch (e)
    {
      name = e.name;
    }

    assert(name === "TypeError");

    if (msg)
    {
      print(msg + " PASS (XFAIL)");
    }
  }

  (function test_incoercible_o(undefined)
  {
    test_set_prototype_of_error(undefined, new Object(), "Object.setPrototypeOf(undefined, ...)");
    test_set_prototype_of_error(null, new Object(), "Object.setPrototypeOf(null, ...)");
  })();
})();
