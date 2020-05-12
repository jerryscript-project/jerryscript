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

/* This test checks await expressions (nothing else). */

function check_syntax_error (code)
{
  try {
    eval (code)
    assert (false)
  } catch (e) {
    assert (e instanceof SyntaxError)
  }
}

check_syntax_error("(async function await() {})")
check_syntax_error("(async function *await() {})")
check_syntax_error("async function f(await) {}")
check_syntax_error("(async function f(await) {})")
check_syntax_error("async function f(a = await new Promise) {}")
check_syntax_error("async function f() { function await() {} }")
check_syntax_error("async await => 0");
check_syntax_error("async (await) => 0");
check_syntax_error("async function f() { await () => 0 }");
check_syntax_error("async (a) => a\\u0077ait a");
check_syntax_error("async (a) => { () => 0\na\\u0077ait a }");

// Valid uses of await

async a => await a
async a => { await a }
async (a) => await a
async(a) => { await a }

// Nested async and non-async functions

async (a) => {
  () => await
  await a
}

(a) => {
  await
  async (a) => await a
  await
  async (a) => await a
  a\u0077ait
}

async function f1(a) {
  await a
  (function () { await ? async function(a) { await a } : await })
  await a
}

async (a) => {
  await a;
  () => await ? async (a) => await a : await
  await a
}

async (a) => {
  (a = () => await, [b] = (c))
  await a
  (a, b = () => await)
  await a
}

// Object initializers

var o = {
  async await(a) {
    await a;
    () => await
    await a
  },

  f(a) {
    await
    async (a) => await a
    await
    async (a) => await a
    a\u0077ait
  },

  async ["g"] () {
    await a;
    () => await
    await a
  }
}

async function f2(a) {
  var o = {
    [await a]() { await % await }
  }
  await a;
}

class C {
  async await(a) {
    await a;
    () => await
    await a
  }

  f(a) {
    await
    async (a) => await a
    await
    async (a) => await a
    a\u0077ait
  }

  async ["g"] () {
    await a;
    () => await
    await a
  }
}

async function f3(a) {
  class C {
    [await a]() { await % await }
  }
  await a;
}
