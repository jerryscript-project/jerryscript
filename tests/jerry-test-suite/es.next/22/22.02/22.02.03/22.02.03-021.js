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

var float64 = new Float64Array(4);
float64.set([10.1, "11.2", 12.3]);
assert(float64[0] === 10.1 && float64[1] === 11.2 && float64[2] === 12.3);

float64.set([13.1, 14.2, 15.3], 1);
assert(float64[0] === 10.1 && float64[1] === 13.1 && float64[2] === 14.2 && float64[3] === 15.3);

try
{
  // 22.2.3.22.1 step 20
  float64.set([17.1, 18.2, 19.3], 2);
  assert(false);
} catch (e)
{
  assert(e instanceof RangeError)
}
