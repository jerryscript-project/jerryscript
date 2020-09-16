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

function check_property(obj, name, value)
{
  property = Object.getOwnPropertyDescriptor(obj, name)
  assert(typeof property === "object")
  assert(property.value === value)
}

var o = {}
var name = "Pro"
var res = 0
var counter = 0

function f1() {
  counter++
}

class C1 {
  static
    v\u0061r
  static Prop =
    res
    =
    "msg"
  static
    Prop
    =
    f1()
  static [name + "p"] = (f1(), o)
  static 22 = 3 * 4  ;static 23 = 5 + 6
  static 'a b'
}

check_property(C1, "var", undefined)
check_property(C1, "Prop", o)
check_property(C1, 22, 12)
check_property(C1, 23, 11)
check_property(C1, "a b", undefined)
assert(res === "msg")
assert(counter === 2)
