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

var testArray = [{0: '0', 1: 0},
                 {0: '1', 1: 1},
                 {0: '2', 1: 2},
                 {0: '3', 1: 3},
                 {0: '4', 1: 4},
                 {0: '5', 1: 5},
                 {0: '6', 1: 6}];

var m = new Map(testArray);

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

var entryIterators = [m.entries(), m[Symbol.iterator]()];
var keyIterator = m.keys();
var valueIterator = m.values();
var elementCount = m.size;

for (var i = 0; i < elementCount; i++) {
  entryIterators.forEach(function(element) {
    var next = element.next();
    assert(next.done === false);
    assert(next.value[0] === '' + i);
    assert(next.value[1] === i);
  });

  var next = keyIterator.next();
  assert(next.done === false);
  assert(next.value === '' + i);

  var next = valueIterator.next();
  assert(next.done === false);
  assert(next.value === i);
}

entryIterators.forEach(function(element) {
  var next = element.next();
  assert(next.done === true);
  assert(next.value === undefined);
});

var next = keyIterator.next();
assert(next.done === true);
assert(next.value === undefined);

next = valueIterator.next();
assert(next.done === true);
assert(next.value === undefined);

var entryIterators = [m.entries(), m[Symbol.iterator]()];
var keyIterator = m.keys();
var valueIterator = m.values();
var elementCount = m.size;

for (var i = 0; i < elementCount; i++) {
  entryIterators.forEach(function(element) {
    var next = element.next();
    assert(next.done === false);
    assert(next.value[0] === '' + i);
    assert(next.value[1] === i);
  });

  var next = keyIterator.next();
  assert(next.done === false);
  assert(next.value === '' + i);

  var next = valueIterator.next();
  assert(next.done === false);
  assert(next.value === i);
  m.delete('' + i);
}

assert(m.size === 0);

m = new Map(testArray);
var loopCount = 0;
var expected = [{0: '0', 1: 0},
                {0: '2', 1: 2},
                {0: '4', 1: 4},
                {0: '6', 1: 6},
                {0: '1', 1: 1},
                {0: '3', 1: 3},
                {0: '5', 1: 5}]

m.forEach(function(value, key) {
  if (loopCount === 0) {
    for (i = 0; i < testArray.length; i++) {
      if (i % 2) {
        m.delete(testArray[i][0]);
        m.set(testArray[i][0], testArray[i][1]);
      }
    }
  }

  assert (key === expected[loopCount][0]);
  assert (value === expected[loopCount][1]);

  loopCount++;
});

assert(loopCount === expected.length);

loopCount = 0;
expected = [{0: '0', 1: 0},
            {0: '1', 1: 1}];

for (var [key, value] of m) {
  if (loopCount === 0) {
    m.clear();
    m.set('1', 1);
  }

  assert(key === expected[loopCount][0]);
  assert(value === expected[loopCount][1]);

  loopCount++;
}

m = new Map(testArray);
loopCount = 0;

for (var [key, value] of m) {
  if (loopCount === 0) {
    m.delete('' + testArray.length - 1);
  }

  assert(key === '' + loopCount);
  assert(value === loopCount);

  loopCount++;
}
