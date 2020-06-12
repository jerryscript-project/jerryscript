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

var a = new Float32Array([-1.1, 0.1, 2.5, 3.0]);
var o = {
  "small":0,
  "large":0
};
var func = function(v, k)
{
  if (v < 2)
  {
    this.small = this.small + k;
  }
  else
  {
    this.large = this.large + k;
  }
}

var ret = a.forEach(func, o);

assert(ret === undefined);
assert(o.small === 1); // 0+1
assert(o.large === 5); // 2+3
