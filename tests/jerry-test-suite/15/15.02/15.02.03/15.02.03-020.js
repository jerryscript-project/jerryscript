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

var emptyObject = {}

var propertyDescriptor = {
  enumerable: true,
  configurable: true,
  value: "hello!",
  writable: true
}

Object.defineProperty(emptyObject, 'myProperty', propertyDescriptor);

var isWritable = false;
var isEnumerable = false;
var isConfigurable = false;

emptyObject.myProperty = "foobar";
if (emptyObject.myProperty == "foobar")
  isWritable = true;

for (p in emptyObject) {
  if (emptyObject[p] == "foobar") {
    isEnumerable = true;
    break;
  }
}

if (delete emptyObject.myProperty)
  isConfigurable = true;

assert(isWritable && isEnumerable && isConfigurable);
