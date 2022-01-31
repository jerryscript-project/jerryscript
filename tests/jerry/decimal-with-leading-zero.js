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

// valid decimal with leading zero
assert (01239 === 1239)
assert (0999 === 999)
assert (0000000000009 === 9)

function invalid_strict_cases (string)
{
  "use strict"
  try {
    eval (string)
    assert (false)
  } catch (e) {
    assert (true)
  }
}

// invalid strict test-cases
invalid_strict_cases ("09")
invalid_strict_cases ("01239")

// invalid to create bigint with decimal with leading zero
function invalid_cases (string)
{
  try {
    eval (string)
    assert (false)
  } catch (e) {
    assert (true)
  }
}

invalid_cases ("09n")
invalid_cases ("01239n")
