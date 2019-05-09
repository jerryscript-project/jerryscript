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

try {
  String.prototype[Symbol.iterator].call(undefined);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  String.prototype[Symbol.iterator].call(Symbol('foo'));
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

var str = 'foobar';
var iterator = str[Symbol.iterator]();

try {
  iterator.next.call(5);
  assert (false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  iterator.next.call({});
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

var str = 'Example string.';
var expected = ['E', 'x', 'a', 'm', 'p', 'l', 'e', ' ', 's', 't', 'r', 'i', 'n', 'g', '.'];

var iterator = str[Symbol.iterator]();
var next = iterator.next();
var idx = 0;

while (!next.done) {
  assert(next.value === expected[idx++]);
  assert(next.done === false);
  next = iterator.next();
}

assert(next.value === undefined);

var str = '^o𓙦һR񼴆]ŭ媖?᯾豼עW򏋁';
var expected = ['^', 'o', '𓙦', 'һ', 'R', '񼴆', ']', 'ŭ', '媖', '?', '᯾', '豼', 'ע', 'W', '򏋁'];

iterator = str[Symbol.iterator]();
next = iterator.next();
idx = 0;

while (!next.done) {
  assert(next.value === expected[idx++]);
  assert(next.done === false);
  next = iterator.next();
}

assert(next.value === undefined);

assert(iterator.toString() === '[object String Iterator]');
assert(iterator[Symbol.toStringTag] === 'String Iterator');
