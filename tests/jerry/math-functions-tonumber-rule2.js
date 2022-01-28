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

var str = "";
var a = {valueOf: function() { str += "a"; return 1;}};
var b = {valueOf: function() { str += "b"; return NaN;}};
var c = {valueOf: function() { str += "c"; return 2;}};
var d = {valueOf: function() { str += "d"; return Infinity;}};
var e = {valueOf: function() { str += "e"; return 3;}};

Math.hypot (a, b, c, d, e);
assert (str === "abcde");
