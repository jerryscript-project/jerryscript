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

var successCount = 0
var p, r

// Test 1

async function f1(p)
{
  assert(await p === 1)
  return 2
}

p = new Promise(function(resolve, reject) { r = resolve })

f1(p).then(function (v) {
  assert(v === 2)
  successCount++
})

r(1)

// Test 2

var f2 = async(p) =>
{
  assert(await p === 3)
}

p = new Promise(function(resolve, reject) { r = resolve })

f2(p).then(function (v) {
  assert(v === undefined)
  successCount++
})

r(3)

// Test 3

var thenableCounter = 0

async function f3()
{
  return new Promise(function(resolve) { resolve(f3) })
}

f3.then = function(resolve) {
  // Repeating resolve with 'then'
  if (++thenableCounter < 5) {
    resolve(f3)
  } else {
    successCount++
  }
}

f3()

// Test 4

async function f4(p)
{
  try {
    throw 4
  } catch (e) {
    throw 5
  }
}

p = new Promise(function(resolve, reject) { r = resolve })

f4(p).then(undefined, function (v) {
  assert(v === 5)
  successCount++
})

r(1)

// Test 5

async function f5(p)
{
  try {
    return 6
  } finally {
    throw 7
  }
}

p = new Promise(function(resolve, reject) { r = resolve })

f5(p).then(undefined, function (v) {
  assert(v === 7)
  successCount++
})

r(1)

// Test 6

p = new Promise(function(resolve, reject) { r = resolve })

async function f6(p)
{
  await p
  return self
}

var self = f6()

self.then(undefined, function (v) {
  assert(v instanceof TypeError)
  successCount++
})

r(1)

// Test 7

async function f7(p)
{
  var x = {}
  assert((await x) === x)

  x = 3.14
  assert((await x) === x)

  x = "Test string"
  assert((await x) === x)

  successCount++
}
f7();

// Test 8

async function f8() {
  var p = new Promise(function() {});
  Object.defineProperty(p, 'constructor', { get() { throw "Error!" } });

  await p
}

f8().then(undefined, function (v) {
  assert(v === "Error!")
  successCount++
})

// END

function __checkAsync() {
  assert(successCount === 8)
  assert(thenableCounter === 5)
}
