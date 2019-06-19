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

var func = function (number) {
  "use strict";
  number.foo = "";
};

var func2 = function (number) {
  "use strict";
  number.bar = "";
};

try {
  func(-334918463);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  Object.defineProperty(Number.prototype, "foo", { get : function() { throw ReferenceError("foo"); },
                                                   set : function(a) { throw ReferenceError("bar"); },
                                                 });
  func(-334918463);
  assert(false);
} catch (e) {
  assert(e instanceof ReferenceError);
  assert(e.message === "bar");
}

var setter_called = false;
Object.defineProperty(Number.prototype, "bar", { get : function() { assert(false) },
                                                 set : function(a) { setter_called = true; },
                                               });
func2(-334918463);
assert(setter_called === true);
