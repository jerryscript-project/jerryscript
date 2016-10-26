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

print("first line of code");

function f1()
{
	function f2()
	{
		return function f3() {
			a = 4;
			print("funciton f3");
		};
	}

	x =
	  6;
}

function f4() {
	print("function f4");
}

function foo()
{
  print("function foo");
  var tmp = 4;
  f4();
}

print ("var cat");
var cat = 'cat';

function test()
{
	print("function test");
	foo();
	var a = 3;
	var b = 5;
	var c = a + b;
	global_var = c;
	return c;
}

var
	x =
	  1;

test();
