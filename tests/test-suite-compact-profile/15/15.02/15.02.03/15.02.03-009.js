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

var a = {
  prop1: Number,
  prop2: String,
  foo: function () {
    return 1;
  },
  bar: function () {
    return 0;
  }
};
names = Object.getOwnPropertyNames(a);

assert(names instanceof Array &&
        names[0] === "prop1" &&
        names[1] === "prop2" &&
        names[2] === "foo" &&
        names[3] === "bar");