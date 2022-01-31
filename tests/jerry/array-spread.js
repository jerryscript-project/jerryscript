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

function assertArrayEqual (actual, expected) {
  assert (actual.length === expected.length);

  for (var i = 0; i < actual.length; i++) {
    assert (actual[i] === expected[i]);
  }
}

function checkSyntax (str) {
  try {
    eval (str);
    assert (false);
  } catch (e) {
    assert (e instanceof SyntaxError);
  }
}

function mustThrow (str) {
  try {
    eval (str);
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }
}

checkSyntax ("{...a}");
checkSyntax ("...a");
checkSyntax ("[...]");
checkSyntax ("[...(...)]");
checkSyntax ("[......]");

mustThrow ("[...5]");
mustThrow ("[...5, 'foo', 'bar']");
mustThrow ("[...{}]");
mustThrow ("[...{ get [Symbol.iterator] () { throw new TypeError } }]");
mustThrow ("[...{ [Symbol.iterator] () {} }, 'foo']");
mustThrow ("[...{ [Symbol.iterator] () { return {} } }]");
mustThrow ("[...{ [Symbol.iterator] () { return { next: 5 } } }]");
mustThrow ("[...{ [Symbol.iterator] () { return { next: 5 } } }], 'foo'");
mustThrow ("[...{ [Symbol.iterator] () { return { get next() { throw new TypeError } } } }]");
mustThrow ("[...{ [Symbol.iterator] () { return { next () { } } } }]");
mustThrow ("[...{ [Symbol.iterator] () { return { next () { } } } }, 'foo']");
mustThrow ("[...{ [Symbol.iterator] () { return { next () { return { get value () { throw new TypeError } } } } } } ]");
mustThrow ("[...{ [Symbol.iterator] () { return { next () { return { get done () { throw new TypeError } } } } } } ]");

var arr1 = [0, 1, 2];
var arr2 = [3, 4, 5];
var arr3 = [{}, {}, {}];
var expected = [0, 1, 2, 3 ,4, 5];

assertArrayEqual ([...arr1, ...arr2], [0, 1, 2, 3 ,4, 5]);
assertArrayEqual ([...arr2, ...arr1], [3 ,4, 5, 0, 1, 2]);
assertArrayEqual ([...arr1, 9, 9, 9, ...arr2], [0, 1, 2, 9, 9, 9, 3 ,4, 5]);
assertArrayEqual ([...arr1, ...[...arr2]], [0, 1, 2, 3 ,4, 5]);
assertArrayEqual (["0" , "1", ...arr1, ...[...arr2]], ["0", "1", 0, 1, 2, 3 ,4, 5]);
assertArrayEqual ([...arr3], arr3);
assertArrayEqual ([..."foobar"], ["f", "o", "o", "b", "a" ,"r"]);
assertArrayEqual ([...(new Set([1, "foo", arr3]))], [1, "foo", arr3]);

var holyArray = [,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,...arr1];
assert (holyArray.length === 83);
assert (holyArray[82] === 2);
assert (holyArray[81] === 1);
assert (holyArray[80] === 0);
