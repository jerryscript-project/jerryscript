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

var i = 10;
var cnt = 0;

do
{
  if (i % 2)
    continue;

  var j = 0;
  do
  {
    if (j % 2 === 0)
      continue;
    cnt++;
  }
  while (j++ < 20)
}
while (i-- > 0);

assert(cnt === 60);