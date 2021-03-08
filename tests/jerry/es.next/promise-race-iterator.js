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

// iterator next
Promise.race({[Symbol.iterator]() { return {get next() { throw 5; }}}
}).catch(err => {
  assert(err === 5);
});

// iterator value
Promise.race({ [Symbol.iterator] () { return { next () { return { get value () { throw 5 }}}}}
}).catch(err => {
  assert(err === 5);
});

// iterator done
Promise.race({ [Symbol.iterator] () { return { next () { return { get done () { throw 5 }}}}}
}).catch(err => {
  assert(err === 5);
});

// iterator get
Promise.race({ get [Symbol.iterator] () { throw 5 }
}).catch(err => {
  assert(err === 5);
});

var fulfills = Promise.race(createIterable([
  new Promise(resolve => { resolve("foo"); }),
  new Promise(resolve => { resolve("bar"); }),
]));
var rejects = Promise.race(createIterable([
  new Promise((_, reject) => { reject("baz"); }),
  new Promise((_, reject) => { reject("qux"); }),
]));

fulfills.then(result => { assert (result + "" === "foo"); });
rejects.catch(result => { assert (result === "baz"); });

var closed = true;
delete Promise.resolve;
Promise.race(createIterable([1,2,3], {'return': function () { closed = false; }}));
assert (closed);

var arr = [];
Object.defineProperty(arr, Symbol.species, { get: function () { assert(false) }});
Promise.race(arr);
