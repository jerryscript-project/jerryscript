// Copyright 2016 Samsung Electronics Co., Ltd.
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

var a = {}, b = 0;

while (a[b]) {
  assert (false);
}

for ( ; a[b]; ) {
  assert (false);
}

var flag = false;
do
{
  assert (!flag);
  flag = true;
} while (a[b]);

a = { };
a.b = { c : 1 };

with (a.b)
{
  assert (c === 1);
}
