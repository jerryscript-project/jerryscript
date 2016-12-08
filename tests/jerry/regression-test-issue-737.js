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

function check_syntax_error (s) {
  try {
    eval (s);
    assert (false);
  } catch (e) {
    assert (e instanceof SyntaxError);
  }
}

/* Test case #1 */
check_syntax_error (
" new function f(f) {                                \
  return {className: 'xxx'};                         \
};                                                   \
x = 1;                                               \
function g(active) {                                 \
  for (i = 1; i <= 1000; i++) { if (i == active) {   \
  x = i;   if (f(\"\" + i) != null) { }              \
    } else {                                         \
  if (f(\"\" + i) != null) }                         \
  }                                                  \
}                                                    \
g(0)                                                 \
");

/* Test case #2 */
check_syntax_error (
" new function a(a) {;for (f in [1,2,3]) print(f);   \
}; 1;                                                \
function g(active) {                                 \
  for (i = 1; i <= 1000; i++) { if (i == active) {   \
  xI                                                 \
      if (f != null) { }                             \
    } else {                                         \
  if (f(\"\" + i) != null) }                         \
  }                                                  \
}                                                    \
g(0)                                                 \
");

/* Test case #3 */
check_syntax_error (
" new function f(f) {;for (f in [1,2,3]) pRint(f);   \
}; 1;                                                \
function g(active) {                                 \
  for (i = 1; i <= 1000; i++) { if (i == active) {   \
  x                                                  \
      if (f != null) { }                             \
    } else {                                         \
  if (f(\"\" + i) != null) }                         \
  }                                                  \
}                                                    \
g(0)                                                 \
");
