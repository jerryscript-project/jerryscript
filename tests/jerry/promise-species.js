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

// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Test that Promises use @@species appropriately

// Another constructor with no species will not be instantiated

var test = new Promise(function(){});
var bogoCount = 0;
function bogusConstructor() { bogoCount++; }
test.constructor = bogusConstructor;
assert(Promise.resolve(test) instanceof Promise);
assert(!(Promise.resolve(test) instanceof bogusConstructor));

// If there is a species, it will be instantiated
// @@species will be read exactly once, and the constructor is called with a
// function
var count = 0;
var params;

class MyPromise extends Promise {
  constructor(...args) {
    super(...args);
    params = args;
  }
  static get [Symbol.species]() {
    count++
    return this;
  }
}

var myPromise = MyPromise.resolve().then();
assert(1 === count);
assert(1 === params.length);
assert('function' === typeof(params[0]));
assert(myPromise instanceof MyPromise);
assert(myPromise instanceof Promise);
