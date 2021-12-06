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

var d = 1
function f(a = function () { return d })
{
  var d = 2
  assert(d === 2)
  assert(a() === 1)
}
f()

var g = (a = () => d) => {
  var d = 2
  assert(d === 2)
  assert(a() === 1)
}
g()

var h = ([{a}] = [{a: function () { return d }}]) => {
  var d = 2
  assert(d === 2)
  assert(a() === 1)
}
h()

function i(a = ((eval))("(function () { return d })"))
{
  var d = 2
  assert(d === 2)
  assert(a() === 1)
}
i()

function j(a = (([1, ((() => d))])[1]))
{
  var d = 2
  assert(d === 2)
  assert(a() === 1)
}
j()

var m = 0
function l(a)
{
  m = a
  return m
}

function k(a = l(() => d))
{
  var d = 2
  assert(d === 2)
  assert(a() === 1)
  assert(m() === 1)
}
k()
