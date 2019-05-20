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

var methods = ['entries', 'keys', 'values', Symbol.iterator];

methods.forEach(function (method) {
  try {
    Map.prototype[method].call(5);
    assert(false);
  } catch (e) {
    assert(e instanceof TypeError);
  }
});

methods.forEach(function (method) {
  try {
    Map.prototype[method].call({});
    assert(false);
  } catch (e) {
    assert(e instanceof TypeError);
  }
});

var m = new Map([{0: '0', 1: 0},
                 {0: '1', 1: 1},
                 {0: '2', 1: 2},
                 {0: '3', 1: 3},
                 {0: '4', 1: 4},
                 {0: '5', 1: 5},
                 {0: '6', 1: 6}]);

methods.forEach(function(method) {
  assert(m[method]().toString() === '[object Map Iterator]');
});

methods.forEach(function (method) {
  try {
    m[method].next.call(5);
    assert(false);
  } catch (e) {
    assert(e instanceof TypeError);
  }
});

methods.forEach(function (method) {
  try {
    m[method].next.call({});
    assert(false);
  } catch (e) {
    assert(e instanceof TypeError);
  }
});

var valueIterators = [m.values(), m[Symbol.iterator]()];
var keyIterator = m.keys();
var entryIterator = m.entries();
var elementCount = m.size;

for (var i = 0; i < elementCount; i++) {
  valueIterators.forEach(function(element) {
    var next = element.next();
    assert(next.done === false);
    assert(next.value === i);
  });

  var next = keyIterator.next();
  assert(next.done === false);
  assert(next.value === '' + i);

  var next = entryIterator.next();
  assert(next.done === false);
  assert(next.value[0] === '' + i);
  assert(next.value[1] === i);
}

valueIterators.forEach(function(element) {
    var next = element.next();
    assert(next.done === true);
    assert(next.value === undefined);
  });

var next = keyIterator.next();
assert(next.done === true);
assert(next.value === undefined);

next = entryIterator.next();
assert(next.done === true);
assert(next.value === undefined);

var valueIterators = [m.values(), m[Symbol.iterator]()];
var keyIterator = m.keys();
var entryIterator = m.entries();
var elementCount = m.size;

for (var i = 0; i < elementCount; i++) {
  valueIterators.forEach(function(element) {
    var next = element.next();
    assert(next.done === false);
    assert(next.value === i);
  });

  var next = keyIterator.next();
  assert(next.done === false);
  assert(next.value === '' + i);

  var next = entryIterator.next();
  assert(next.done === false);
  assert(next.value[0] === '' + i);
  assert(next.value[1] === i);
  m.delete('' + i);
}

assert(m.size === 0);
