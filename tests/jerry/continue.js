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

for (var i = 0; i < 1000; i++)
{
  switch (1)
  {
  default:
    /* This block must not be enclosed in braces. */
    let j = eval();
    continue;
  }
}

next:
for (var i = 0; i < 1000; i++)
{
  for (let j = eval(); true; )
  {
    continue next;
  }
}

next:
for (var i = 0; i < 1000; i++)
{
  for (let j in {a:1})
  {
    eval()
    continue next;
  }
}
