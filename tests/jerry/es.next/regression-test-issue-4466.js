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

var called_get = false;
var targetProxy = new Proxy({}, {
  getOwnPropertyDescriptor: function(target, key) {
    throw new URIError("Generate error");
  }
});

var px = new Proxy(targetProxy, {
  get: function(target, key) {
    called_get = true;
    return { debug: 1 };
  }
});

try {
  px.abc;
  assert(false);
} catch (ex) {
  assert(ex instanceof URIError);
  assert(called_get === true);
}
