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

print("client-source-test");

function finish(z) {
  print("function finish");
  print("finish: " + z);
}

function bar(y) {
  print("function bar");
  finish(y + "-bar");
}

function foo(x)
{
  print("function foo");
  bar(x + "-foo");
}

function test()
{
  print("function test");
  var x = "test";
  foo(x);
}

test();
