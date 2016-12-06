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

function re_test (pattern, string, expected)
{
  assert ((new RegExp(pattern)).exec(string) == expected);
}

re_test("[\\u0020]", "u", null);
re_test("[\\u0020]", " ", " ");
re_test("[\\u0020]", "x", null);

re_test("[\\x20]", "u", null);
re_test("[\\x20]", " ", " ");
re_test("[\\x20]", "x", null);
