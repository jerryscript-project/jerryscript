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

var i = 10;
var cnt = 0;

do
{
  var j = 0;
  do
  {
    if (j % 2 === 0)
      break;
    cnt++;
  }
  while (j++ < 20)

  if (i % 2)
    break;
}
while (i-- > 0);

assert(cnt === 0);
