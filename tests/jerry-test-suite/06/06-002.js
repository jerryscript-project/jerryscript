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

function c(arg)
{
  var obj = new Object();
  obj.print = function () {
    f = arg;
  };
  return obj;
}

a = c(5);
b = c(6);

a.print.toString = 7;

assert(typeof a.print.toString !== typeof b.print.toString);
