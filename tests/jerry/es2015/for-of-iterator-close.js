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

function createIterable(arr, methods = {}) {
  let iterable = function *() {
    let idx = 0;
    while (idx < arr.length) {
      yield arr[idx];
      idx++;
    }
  }();
  iterable['return'] = methods['return'];
  iterable['throw'] = methods['throw'];

  return iterable;
};

function close1() {
  var closed = false;
  var iter = createIterable([1, 2, 3], {
    'return': function() { closed = true; return {}; }
  });
  for (var it of iter) break;
  return closed;
}

assert(close1());

function close2() {
  var closed = false;
  var iter = createIterable([1, 2, 3], {
    'return': function() { closed = true; return {}; }
  });
  try {
    for (var it of iter) throw 0;
    assert(false);
  } catch(e){
    assert(e === 0);
  }
  return closed;
}

assert(close2());

function close3() {
  var closed = false;
  var iter = createIterable([1, 2, 3], {
    'return': function() { closed = true; return {}; }
  });
  for (var it of iter) continue;
  return closed;
}

assert(!close3());

function close4() {
  var closed = false;
  var iter = createIterable([1, 2, 3], {
    'return': function() { closed = true; throw 6; }
  });
  try {
    for (var it of iter) throw 5;
    assert(false);
  } catch(e) {
    assert(e === 5);
  }
  return closed;
}

assert(close4());

function close5() {
  var closed_called = 0;
  var iter = createIterable([1, 2, 3], {
    'return': function() { closed_called++; throw 6; }
  });
  try {
    for (var it of iter) {
      for (var it of iter) {
        throw 5;
      }
      assert(false);
    }
    assert(false);
  } catch(e) {
    assert(e === 5);
  }
  return closed_called === 2;
}

assert(close5());

function close6() {
  var closed = false;
  var iter = createIterable([1, 2, 3], {
    'return': function() { closed = true; return {}; }
  });
  for (var it of iter) {};

  return closed;
}

assert(!close6());

var close7_result = false;
function close7() {
  var iter = createIterable([1, 2, 3], {
    'return': function() { close7_result = true; throw "5"; }
  });

  for (var it of iter) {
    return "foo";
  }
}

try {
  close7();
  assert(false);
} catch (e) {
  assert(close7_result);
  assert(e === "5");
}

function close8() {
  var iter = createIterable([1, 2, 3], {
    'return': function() { close8_result = true; throw "5"; }
  });

  for (var it of iter) {
    throw "foo";
  }
}

var close8_result = false;
try {
  close8();
  assert(false);
} catch (e) {
  assert(e === "foo");
  assert(close8_result);
}

function close9() {
  var closed = false;
  var iter = createIterable([1, 2, 3], {
    'return': function() { closed = true; throw "5"; }
  });

  try {
    for (var it of iter) {
      break;
    }
  } finally {
    assert(closed);
    throw "foo"
  }
}

try {
  close9();
  assert(false);
} catch (e) {
  assert(e === "foo");
}

function close10() {
  var closed = false;
  var iter = createIterable([1, 2, 3], {
    'return': function() { closed = true; return {}; }
  });

  try {
    for (var it of iter) {
      return "foo";
    }
  } finally {
    assert(closed);
    throw "bar";
  }
}

try {
  close10();
  assert(false);
} catch (e) {
  assert(e === "bar");
}

function close11() {
  var closed = false;
  var iter = createIterable([1, 2, 3], {
    'return': function() { closed = true; throw "5"; }
  });

  try {
    for (var it of iter) {
      return "foo";
    }
  } finally {
    assert(closed);
    throw "bar";
  }
}

try {
  close11();
  assert(false);
} catch (e) {
  assert(e === "bar");
}

function close12() {
  var closed = false;
  var iter = createIterable([1, 2, 3], {
    'return': function() { closed = true; throw "5"; }
  });

  try {
    for (var it of iter) {
      throw "foo";
    }
  } finally {
    assert(closed);
    throw "bar";
  }
}

try {
  close12();
  assert(false);
} catch (e) {
  assert(e === "bar");
}
