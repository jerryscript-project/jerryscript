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

// Test array
var arr = ['a', 'b', 'c'];
var props = Object.keys(arr);
// props should contain: 0,1,2 and the order is not defined!
assert (props.indexOf("0") !== -1);
assert (props.indexOf("1") !== -1);
assert (props.indexOf("2") !== -1);
assert (props.length === 3);

// Test object
var obj = {key1: 'a', key3: 'b', key2: 'c', key4: 'c', key5: ''};
props = Object.keys(obj);
// props should contain: key1,key2,key3,key4,key5 and the order is not defined!
assert (props.indexOf("key1") !== -1);
assert (props.indexOf("key2") !== -1);
assert (props.indexOf("key3") !== -1);
assert (props.indexOf("key4") !== -1);
assert (props.indexOf("key5") !== -1);
assert (props.length === 5);

var obj2 = {};
Object.defineProperties(obj2, {
    key_one: {enumerable: true, value: 'one'},
    key_two: {enumerable: false, value: 'two'},
});

props = Object.keys(obj2);
// props should contain: key_one
assert (props.indexOf("key_one") !== -1);
assert (props.indexOf("key_two") === -1);
assert (props.length === 1);

// Test prototype chain
function Parent() {}
Parent.prototype.inheritedMethod = function() {};

function Child() {
  this.prop = 5;
  this.method = function() {};
}
Child.prototype = new Parent;
Child.prototype.prototypeMethod = function() {};

props = Object.keys (new Child());
// props should contain: prop,method and the order is not defined!
assert (props.indexOf("prop") !== -1);
assert (props.indexOf("method") !== -1);
assert (props.length === 2);

// Test non-object argument
try {
  Object.keys("hello");
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

var o = {};

Object.defineProperty(o, 'a', {
  value: "OK",
  writable: true,
  enumerable: true,
  configurable: true
});

Object.defineProperty(o, 'b', {
  value: "NOT_OK",
  writable: true,
  enumerable: false,
  configurable: true
});

Object.defineProperty(o, 'c', {
  value: "OK",
  writable: true,
  enumerable: true,
  configurable: true
});

props = Object.keys(o);
assert(props.length === 2);
assert(o[props[0]] === "OK");
assert(o[props[1]] === "OK");
