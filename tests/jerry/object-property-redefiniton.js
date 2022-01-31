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

function checkOwnProperties(obj, propList)
{
  names = Object.getOwnPropertyNames(obj)
  assert(names.length === propList.length)

  for (var i = 0; i < propList.length; ++i)
  {
    assert(names[i] === propList[i])

    var descriptor = Object.getOwnPropertyDescriptor(obj, names[i])
    if (i % 2 == 0) {
      assert(descriptor.writable == true);
      assert(descriptor.enumerable == true);
      assert(descriptor.configurable == true);
      assert(descriptor.get === undefined);
      assert(descriptor.set === undefined);
    } else {
      assert(descriptor.writable == undefined);
      assert(descriptor.enumerable == true);
      assert(descriptor.configurable == true);
      assert(descriptor.get !== undefined || descriptor.set !== undefined);
    }
  }
}

var o = {
  get a() {},
  b:6,
  set c(_) {
  },
  d:10,
  a: 11,
  get b () {},
  c: 12,
  set d (_) {}
}

checkOwnProperties(o, ['a', 'b', 'c', 'd']);
