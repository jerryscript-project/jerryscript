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

function parse (txt) {
  try {
    eval (txt)
    assert (false)
  } catch (e) {
    assert (e instanceof SyntaxError)
  }
}

function checkError (obj) {
  try {
    for (var a of obj);
    assert (false)
  } catch (e) {
    assert (e instanceof TypeError)
  }
}

var arr = [1,2,3,4]

var forOf =
  "for var prop of obj" +
  "   obj [prop] += 4"
parse (forOf)

var forOf =
  "for [var prop of obj]" +
  "   obj[prop] += 4;"
parse (forOf)

var forOf =
  "for (var prop obj)" +
  "   obj[prop] += 4;"
parse (forOf)

var forOf =
  "foreach (var prop of obj)" +
  "   obj[prop] += 4;"
parse (forOf)

var forOf =
  "for (var a \"of\" []) {}"
parse (forOf)

var forOf =
  "for (let x of [], []) {}"
parse (forOf)

var forOf =
  "for (var x of [], []) {}"
parse (forOf)

checkError(7)

var obj = {}
Object.defineProperty(obj, Symbol.iterator, { get : function () { throw TypeError ('foo');}});
checkError (obj);

var obj = {
  [Symbol.iterator] : 5
}

checkError (obj);

var obj = {
  [Symbol.iterator] () {
    return 5
  }
}

checkError(obj);

var obj = {
  [Symbol.iterator] () {
    return {}
  }
}
checkError(obj);

var obj = {
  [Symbol.iterator] () {
    return {
      next() {
        return 5;
      }
    }
  }
}
checkError(obj);

var array = [0, 1, 2, 3, 4, 5];

var i = 0;
for (var a of array) {
  assert (a === i++);
}

var obj = {
  [Symbol.iterator]() {
    return {
      counter : 0,
      next () {
        if (this.counter == 10) {
          return { done : true, value : undefined };
        }
        return { done: false, value: this.counter++ };
      }
    }
  }
}

var i = 0;
for (var a of obj) {
  assert (a === i++);
}

var status = 0;
i = 0;

function yieldNext() {
  ++status
  assert(status === 3 || status === 6 || status === 9)
  return {
    get value() {
      ++status
      assert(status === 4 || status === 7)
      return "Res"
    },
    done: ++i >= 3
  }
}

obj = {
  [Symbol.iterator]() {
    assert(++status === 1)
    return {
      get next() {
        assert(++status === 2)
        return yieldNext
      }
    }
  }
}

function getX() {
  ++status
  assert(status === 5 || status === 8)
  return { x:0 }
}

for (getX().x of obj)
  ;
assert(status == 9)
