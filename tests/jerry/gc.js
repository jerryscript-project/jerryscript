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

var obj = {}, obj_l;
obj_l = obj;

for (var k = 0; k < 1500; k++)
{
  obj_l.prop = {};
  obj_l = obj_l.prop;
}

function f (o, i) {
  if (--i > 0) {
    f ({a:o, b:o}, i);
  }
}

for (var i = 0; i < 100; i++)
{
  ({} + f ({}, 12));
}
