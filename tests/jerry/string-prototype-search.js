// Copyright 2015 University of Szeged
// Copyright 2015 Samsung Electronics Co., Ltd.
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

assert ("abcabbcd".search (/abb+c/) === 3);
assert ("ababbccabd".search ("((?:(ax))|(bx)|ab*c+)") === 2);
assert ("acbaabcabcabc".search (/b+c/g) === 5);
assert ("abcabd".search ("c?a+d") === -1);

assert (String.prototype.search.call ({}, "ec+t") === 4);

try
{
  String.prototype.search.call (null, "u");
  assert (false);
}
catch (e)
{
  assert (e instanceof TypeError);
}

var regexp = /x/g;
regexp.lastIndex = "index";

assert ("aaxbb".search (regexp) === 2);
assert ("aabb".search (regexp) === -1);
assert (regexp.lastIndex === "index");

Object.defineProperty(regexp, "lastIndex", {
  configurable : false,
  enumerable : false,
  value : "index2",
  writable : false
});

assert ("axb".search (regexp) === 1);
assert ("aabb".search (regexp) === -1);
assert (regexp.lastIndex === "index2");

assert ("##\ud801\udc00".search ("\ud801") === 2);
assert ("##\ud801\udc00".search ("\udc00") === 3);

// The real "exec" never returns with a number.
Object.getPrototypeOf(/x/).exec = function () { return "???"; }

assert (/y/.exec("y") === "???");

// Changing exec should not affect search.
assert ("ay".search (/y/) === 1);
