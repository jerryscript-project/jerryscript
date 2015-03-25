// Copyright 2015 Samsung Electronics Co., Ltd.
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

// TODO: Test other field types
// TODO: Test functions

assert(Jerry.io.platform === "linux");

Jerry.io.print_uint32 (1);
Jerry.io.print_string (Jerry.io.platform);

try
{
  // Argument type mismatch
  Jerry.io.print_uint32 ('1');

  assert (false);
}
catch (e)
{
  assert (e instanceof TypeError);
}

try
{
  // Argument type mismatch
  Jerry.io.print_string (1);

  assert (false);
}
catch (e)
{
  assert (e instanceof TypeError);
}
