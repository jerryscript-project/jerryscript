// Copyright 2014 Samsung Electronics Co., Ltd.
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

var properties = {
  prop1: {
    writable: true,
    enumerable: true,
    configurable: false,
    value: "I'm prop1"
  },
  prop2: {
    writable: true,
    enumerable: true,
    configurable: false,
    value: "I'm prop2"
  }
}

var isEnumerable = true;
var isConfigurable = true;
var isWritable = false;

Object.defineProperties(emptyObject, properties);

emptyObject.prop1 = "hello";
emptyObject.prop2 = "world";

if (emptyObject.prop1 === "hello" && emptyObject.prop2 == "world")
  isWritable = true;

for (p in emptyObject) {
  if (emptyObject[p] === "hello")
    isEnumerable = !isEnumerable;
  else if (emptyObject[p] === "world")
    isEnumerable = !isEnumerable;
}

isConfigurable = delete emptyObject.prop1 && delete emptyObject.prop2

assert(isWritable && isEnumerable && !isConfigurable);
