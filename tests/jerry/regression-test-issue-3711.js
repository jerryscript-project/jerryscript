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

function func() {}

var bound = func.bind();

/* original test case from the issue report */
if (function() {
  return func.bind()(0, 0, 0, 0, 0, 0, 0)
}());

/* various versions of the issue report */

/* call the bound function with a lots of args */
for (var idx = 0; idx < 50; idx++) {
    var args = new Array(idx);

    bound.apply(undefined, args);

    delete args;
}

/* bind the function with multiple args and invoke it */
for (var idx = 0; idx < 25; idx++) {
    var args = new Array(idx);

    func.bind.apply(func, args).apply(undefined, args);

    delete args;
}
