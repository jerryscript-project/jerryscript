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

var a = new String("qwe");

names = Object.getOwnPropertyNames(a);

assert(names instanceof Array);

var is_0 = false, is_1 = false, is_2 = false, is_length = false;
for (var i = 0; i <= 3; i++)
{
  if (names[i] === "0") { is_0 = true; }
  if (names[i] === "1") { is_1 = true; }
  if (names[i] === "2") { is_2 = true; }
  if (names[i] === "length") { is_length = true; }
}

assert (is_0 && is_1 && is_2 && is_length);
