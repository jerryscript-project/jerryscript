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

// Test the ES2015 @@species feature

'use strict';

// Subclasses of Array construct themselves under map, etc

class MyArray extends Array { }

function assertEquals(a, b) {
  assert(a === b);
}

assertEquals(MyArray, new MyArray().map(()=>{}).constructor);
assertEquals(MyArray, new MyArray().filter(()=>{}).constructor);
assertEquals(MyArray, new MyArray().slice().constructor);
assertEquals(MyArray, new MyArray().splice().constructor);
assertEquals(MyArray, new MyArray().concat([1]).constructor);
assertEquals(1, new MyArray().concat([1])[0]);

// Subclasses can override @@species to return the another class

class MyOtherArray extends Array {
  static get [Symbol.species]() { return MyArray; }
}

assertEquals(MyArray, new MyOtherArray().map(()=>{}).constructor);
assertEquals(MyArray, new MyOtherArray().filter(()=>{}).constructor);
assertEquals(MyArray, new MyOtherArray().slice().constructor);
assertEquals(MyArray, new MyOtherArray().splice().constructor);
assertEquals(MyArray, new MyOtherArray().concat().constructor);

// Array  methods on non-arrays return arrays

class MyNonArray extends Array {
  static get [Symbol.species]() { return MyObject; }
}

class MyObject { }

assertEquals(MyObject,
             Array.prototype.map.call(new MyNonArray(), ()=>{}).constructor);
assertEquals(MyObject,
             Array.prototype.filter.call(new MyNonArray(), ()=>{}).constructor);
assertEquals(MyObject,
             Array.prototype.slice.call(new MyNonArray()).constructor);
assertEquals(MyObject,
             Array.prototype.splice.call(new MyNonArray()).constructor);
assertEquals(MyObject,
             Array.prototype.concat.call(new MyNonArray()).constructor);

assertEquals(undefined,
             Array.prototype.map.call(new MyNonArray(), ()=>{}).length);
assertEquals(undefined,
             Array.prototype.filter.call(new MyNonArray(), ()=>{}).length);
// slice, splice, and concat actually do explicitly define the length.
assertEquals(0, Array.prototype.slice.call(new MyNonArray()).length);
assertEquals(0, Array.prototype.splice.call(new MyNonArray()).length);
assertEquals(1, Array.prototype.concat.call(new MyNonArray(), ()=>{}).length);

// Defaults when constructor or @@species is missing or non-constructor

class MyDefaultArray extends Array {
  static get [Symbol.species]() { return undefined; }
}
assertEquals(Array, new MyDefaultArray().map(()=>{}).constructor);

class MyOtherDefaultArray extends Array { }
assertEquals(MyOtherDefaultArray,
             new MyOtherDefaultArray().map(()=>{}).constructor);
MyOtherDefaultArray.prototype.constructor = undefined;
assertEquals(Array, new MyOtherDefaultArray().map(()=>{}).constructor);
assertEquals(Array, new MyOtherDefaultArray().concat().constructor);

// Exceptions propagated when getting constructor @@species throws

class SpeciesError extends Error { }
class ConstructorError extends Error { }
class MyThrowingArray extends Array {
  static get [Symbol.species]() { throw new SpeciesError; }
}

function assertThrows (a, b) {
  try {
    a();
  } catch (e) {
    assert(e instanceof b);
  }
}

assertThrows(() => new MyThrowingArray().map(()=>{}), SpeciesError);
Object.defineProperty(MyThrowingArray.prototype, 'constructor', {
    get() { throw new ConstructorError; }
});
assertThrows(() => new MyThrowingArray().map(()=>{}), ConstructorError);

// Previously unexpected errors from setting properties in arrays throw

class FrozenArray extends Array {
  constructor(...args) {
    super(...args);
    Object.freeze(this);
  }
}
assertThrows(() => new FrozenArray([1]).map(()=>0), TypeError);
assertThrows(() => new FrozenArray([1]).filter(()=>true), TypeError);
assertThrows(() => new FrozenArray([1]).slice(0, 1), TypeError);
assertThrows(() => new FrozenArray([1]).splice(0, 1), TypeError);
assertThrows(() => new FrozenArray([]).concat([1]), TypeError);

// Verify call counts and constructor parameters

var count;
var params;
class MyObservedArray extends Array {
  constructor(...args) {
    super(...args);
    params = args;
  }
  static get [Symbol.species]() {
    count++
    return this;
  }
}

function assertArrayEquals(value, expected, type) {
  assert(expected.length === value.length);
  for (var i=0; i<value.length; ++i) {
    assertEquals(expected[i], value[i]);
  }
}

count = 0;
params = undefined;
assertEquals(MyObservedArray,
             new MyObservedArray().map(()=>{}).constructor);
assertEquals(1, count);
assertArrayEquals([0], params);

count = 0;
params = undefined;
assertEquals(MyObservedArray,
             new MyObservedArray().filter(()=>{}).constructor);
assertEquals(1, count);
assertArrayEquals([0], params);

count = 0;
params = undefined;
assertEquals(MyObservedArray,
             new MyObservedArray().concat().constructor);
assertEquals(1, count);
assertArrayEquals([0], params);

count = 0;
params = undefined;
assertEquals(MyObservedArray,
             new MyObservedArray().slice().constructor);
assertEquals(1, count);
assertArrayEquals([0], params);

count = 0;
params = undefined;
assertEquals(MyObservedArray,
             new MyObservedArray().splice().constructor);
assertEquals(1, count);
assertArrayEquals([0], params);
