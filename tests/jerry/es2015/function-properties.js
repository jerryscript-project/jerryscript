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

function getProperties(obj)
{
  var str = "";
  for (name in obj)
  {
    if (str)
    {
      str += " " + name;
    }
    else
    {
      str = name;
    }
  }
  return str;
}

var prototype_obj = { dummy:1, length:1, caller:null,
                      arguments:null, prototype:null };

var func = function() {};

Object.setPrototypeOf(func, prototype_obj);
assert(getProperties(func) == "dummy caller arguments");

var bound_func = (function() {}).bind(null);
Object.setPrototypeOf(bound_func, prototype_obj);
assert(getProperties(bound_func) == "dummy prototype");

// 'print' is an external function
Object.setPrototypeOf(print, prototype_obj);
assert(getProperties(print) == "dummy length caller arguments");
