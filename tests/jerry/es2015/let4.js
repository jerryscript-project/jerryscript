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

var g = -1;

function f1() {
  /* Function hoisted as var. */
  assert (g === undefined);

  {
    assert (g() === 1);

    {
      eval("assert (g() === 2)");

      function g() { return 2 };
    }

    function g() { return 1 };

    assert (g() === 2);
  }

  assert (g() === 2);
}
f1();

function f2() {
  /* Function hoisted as let. */
  assert (g === -1);

  {
    let g = 1;

    {
      if (true)
      {
        assert (g() === 2);

        if (true)
        {
          eval("assert (g() === 2)");
        }

        function g() { return 2 };
      }

      assert (g === 1);
    }

    assert (g === 1);
  }

  assert (g === -1);
}
f2();
