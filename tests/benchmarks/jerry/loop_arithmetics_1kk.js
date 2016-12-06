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

var count = 1000000;
var x = 7;
var y = 3;

var tmp1;
var tmp2;
var tmp3;
var tmp4;

for (var i = 0; i < count; i++)
{
  tmp1 = x * x;
  tmp2 = y * y;
  tmp3 = tmp1 * tmp1;
  tmp4 = tmp2 * tmp2;
}
