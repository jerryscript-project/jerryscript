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

var a = new Array(1, 2, 3, 4, 5, 6, 7);
a.eight = 8;

var p;
for (p in a)
{
  a[p] += 1;
}

assert(a[0] === 2 && a[1] === 3 && a[2] === 4 && a[3] === 5 &&
        a[4] === 6 && a[5] === 7 && a[6] === 8 && a['eight'] === 9);
