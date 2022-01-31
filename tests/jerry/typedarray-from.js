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

var typedArrayConstructors = [
    Uint8Array,
    Int8Array,
    Uint16Array,
    Int16Array,
    Uint32Array,
    Int32Array,
    Uint8ClampedArray,
    Float32Array,
    Float64Array
];

var set = new Set([1,2,3]);

var foo = {};
var bar = {};
var weak_set = new WeakSet();
weak_set.add(foo);
weak_set.add(bar);

var string = new String('123');

var map = new Map();
map.set(0, 'zero');
map.set(1, 'one');

var weak_map = new WeakMap();
weak_map.set(foo, 'foo');
weak_map.set(bar, 'bar');

var string = '123';

for (var constructor of typedArrayConstructors)
{
    assert(constructor.from.length === 1);

    try {
      function f() {constructor.from.call(Array, []);}
    } catch (e) {
      assert(e instanceof TypeError); 
    }

    assert(constructor.from(set).toString() === '1,2,3');
    assert(constructor.from(weak_set).toString() === '');

    if (constructor == Float32Array || constructor == Float64Array)
    {
        assert(constructor.from(map).toString() === 'NaN,NaN');
    }
    else
    {
        assert(constructor.from(map).toString() === '0,0');
    }

    assert(constructor.from(weak_map).toString() === '');
    assert(constructor.from(string).toString() === '1,2,3');

    try {
      function f() {constructor.from.call({}, []);}
    } catch (e) {
      assert(e instanceof TypeError);
    }

    try {
      function f() {constructor.from.call([], []);}
    } catch (e) {
      assert(e instanceof TypeError);
    }

    try {
      function f() {constructor.from.call(1, []);}
    } catch (e) {
      assert(e instanceof TypeError);
    }

    try {
      function f() {constructor.from.call(undefined, []);}
    } catch (e) {
      assert(e instanceof TypeError);
    }

    assert(constructor.from([1,2,3,4]).toString() === '1,2,3,4');
    assert(constructor.from([12,45]).toString() === '12,45');
    assert(constructor.from(NaN).toString() === '');
    assert(constructor.from(Infinity).toString() === '');

    assert(constructor.from([4,5,6], x => x + 1).toString() === '5,6,7');
    assert(constructor.from([2,4,8], x => x * 2).toString() === '4,8,16');

    try {
      constructor.from([1,2,3], x => {throw 5});
      assert(false);
    } catch (e) {
      assert(e === 5);
    }

    try {
      constructor.from([Symbol.match]);
      assert(false);
    } catch (e) {
      assert(e instanceof TypeError);
    }

    try {
      constructor.from([1,1,1], 'foo');
      assert(false);
    } catch (e) {
      assert (e instanceof TypeError);
    }

    try {
      function f() {constructor.from(null)}
    } catch (e) {
      assert(e instanceof TypeError);
    }

    try {
      function f() {constructor.from(undefined);}
    } catch (e) {
      assert(e instanceof TypeError);
    }

    var called = 0;
    var arr = [1,2,3];
    var obj = {};

    function testIterator() {
      called++;
      return arr[Symbol.iterator]();
    }

    var getCalled = 0;
    Object.defineProperty(obj, Symbol.iterator, {
      get: function() {
          getCalled++;
          return testIterator;
      },
    });

    assert(constructor.from(obj).toString() === '1,2,3');
    assert(called === 1);
    assert(getCalled === 1);
}
