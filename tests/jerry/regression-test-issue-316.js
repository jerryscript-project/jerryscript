// Copyright 2015 Samsung Electronics Co., Ltd.
// Copyright 2015 University of Szeged.
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

try {
  v_1 = RegExp.prototype.test;
  v_1(ReferenceError);
}
catch (e)
{
  assert (e instanceof TypeError);
}

try {
  v_1 = RegExp.prototype.exec;
  v_1(ReferenceError);
}
catch (e)
{
  assert (e instanceof TypeError);
}

try {
  v_1 = RegExp.prototype.toString;
  v_1(ReferenceError);
}
catch (e)
{
  assert (e instanceof TypeError);
}
