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

function f1()
{
  function f2()
    {
      assert(k > 0);
      assert(i < 10000);

      if(--k == 0)
        {
          return;
        }

      f2();
    }

  k = 17;

  f2();
}

var k;
var i;

for(i = 0; i < 100; i++)
{
  f1();
}
