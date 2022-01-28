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
    Set.prototype[method].call(5);
    assert(false);
  } catch (e) {
    assert(e instanceof TypeError);
  }
});

methods.forEach(function (method) {
  try {
    Set.prototype[method].call({});
    assert(false);
  } catch (e) {
    assert(e instanceof TypeError);
  }
});

var s = new Set([0, 1, 2, 3, 4, 5, 6]);

methods.forEach(function(method) {
  assert(s[method]().toString() === '[object Set Iterator]');
});

methods.forEach(function (method) {
  try {
    s[method].next.call(5);
    assert(false);
  } catch (e) {
    assert(e instanceof TypeError);
  }
});

methods.forEach(function (method) {
  try {
    s[method].next.call({});
    assert(false);
  } catch (e) {
    assert(e instanceof TypeError);
  }
});

var setFromSet = new Set(s);
assert(setFromSet.size === 7);

var iterators = [s.keys(), s.values(), s[Symbol.iterator]()];
var entryIterator = s.entries();
var elementCount = s.size;

for (var i = 0; i < elementCount; i++) {
  iterators.forEach(function(element) {
    var next = element.next();
    assert(next.done === false);
    assert(next.value === i);
  });

  var next = entryIterator.next();
  assert(next.done === false);
  assert(next.value[0] === i);
  assert(next.value[1] === i);
}

iterators.forEach(function(element) {
    var next = element.next();
    assert(next.done === true);
    assert(next.value === undefined);
  });

var next = entryIterator.next();
assert(next.done === true);
assert(next.value === undefined);


iterators = [s.keys(), s.values(), s[Symbol.iterator]()];
entryIterator = s.entries();
var elementCount = s.size;

for (var i = 0; i < elementCount; i++) {
  iterators.forEach(function(element) {
    var next = element.next();
    assert(next.done === false);
    assert(next.value === i);
  });

  var next = entryIterator.next();
  assert(next.done === false);
  assert(next.value[0] === i);
  assert(next.value[1] === i);
  s.delete(i);
}

assert(s.size === 0);

s = new Set ([0, 1]);
var expected = [0, 1, 2, 4, 5, 6, 3];
var loopCount = 0;

s.forEach(function(element) {
  if (loopCount === 0) {
    for (var i = 0; i < expected.length ; i++) {
      s.add(i);
    }
    s.delete(3);
    s.add(3);
  }
  assert(element === expected[loopCount++]);
});

assert(loopCount === expected.length);

s = new Set([0, 1, 2, 3, 4, 5, 6]);
expected = [0, 1];
loopCount = 0;

for (var value of s) {
  if (loopCount === 0) {
    s.clear();
    s.add(1);
  }

  assert(value === expected[loopCount++]);
}

s = new Set([0])
expected = [0, 1];
loopCount = 0;

for (var value of s) {
  if (loopCount === 0) {
    s.add(2);
    s.delete(2);
    s.add(3);
    s.delete(3);
    s.add(1);
  }

  assert(value === expected[loopCount++]);
}

assert(Set.prototype.values === Set.prototype.keys);
