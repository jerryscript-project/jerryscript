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

// TODO: Update these tests when the internal routine has been implemented

var target = {};
var handler = { deleteProperty (target) {
  throw 42;
}, get (object, propName) {
  if (propName == "length") {
    return 5;
  }
}};

var proxy = new Proxy(target, handler);

var a = 5;

try {
  // ecma_op_delete_binding
  with (proxy) {
    delete a
  }
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  // 22.1.3.16.6.e
  Array.prototype.pop.call(proxy)
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}
