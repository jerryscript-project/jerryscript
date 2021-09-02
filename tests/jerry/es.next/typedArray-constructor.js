/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

function check_range_error(code)
{
  try {
    eval(code);
    assert(false);
  } catch (e) {
    assert(e instanceof RangeError);
  }
}

// Invalid cases

check_range_error("new Uint8Array(1e15)");
check_range_error("new Uint8Array(4294967296)");
check_range_error("new Uint8Array(4294967296 - 1)");
check_range_error("new Uint8Array(-1)");

// Valid cases
assert((new Uint8Array(3.5)).length === 3)
assert((new Uint8Array(-0.5)).length === 0)
