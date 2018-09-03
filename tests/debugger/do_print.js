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

print("Hello world!");
print("A", "[", "", "]", 1.1e2, null, true, undefined);
print();
print("[A\nB", "C\nD", "E\nF\nG]");

var s = "";

for (i = 1; i <= 100; i++) {
  /* Translated from hungarian: life is beautiful */
  s += i + ": Az élet gyönyörű.\n";
}

/* Long string with non-ascii characters. */
print(s);
