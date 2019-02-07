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

// Test array
var a = Symbol ('a');
var b = Symbol ('b');
var c = Symbol ('c');
var d = Symbol ();

var arr = [a, b, c, d];
var props = Object.getOwnPropertySymbols (arr);
// props should not contain: a, b, c, d
assert (props.indexOf ('0') === -1);
assert (props.indexOf ('1') === -1);
assert (props.indexOf ('2') === -1);
assert (props.indexOf ('length') === -1);
assert (props.length === 0);

// Test object
var obj = {};
obj[a] = 'a';
obj[b] = 'b';
obj[c] = 'c';
obj[d] = 'd';
props = Object.getOwnPropertySymbols (obj);
// props should contain: a, b, c, d and the order is not defined!
assert (props.indexOf(a) !== -1);
assert (props.indexOf(b) !== -1);
assert (props.indexOf(c) !== -1);
assert (props.indexOf(d) !== -1);
assert (props.length === 4);

// Test same descriptions
var fooSymbol1 = Symbol ('foo');
var fooSymbol2 = Symbol ('foo');
var fooSymbol3 = Symbol ('foo');
var fooSymbol4 = Symbol ('foo');

var obj = {}
obj[fooSymbol1] = 'foo';
obj[fooSymbol2] = 'bar';
obj[fooSymbol3] = 'foobar';
obj[fooSymbol4] = 'barfoo';

props = Object.getOwnPropertySymbols (obj);
assert (props.indexOf (fooSymbol1) !== -1);
assert (props.indexOf (fooSymbol2) !== -1);
assert (props.indexOf (fooSymbol3) !== -1);
assert (props.indexOf (fooSymbol4) !== -1);
assert (props.length === 4);

var mixed_object = {};
var foo = Symbol ('foo');
var bar = Symbol.for ('bar');

mixed_object[foo] = 'localSymbol';
mixed_object[bar] = 'globalSymbol';
mixed_object['foo'] = 'string';

var props = Object.getOwnPropertySymbols (mixed_object);

assert (typeof props[0] === 'symbol')
assert (props.indexOf(foo) !== -1);
assert (props.indexOf(bar) !== -1);
assert (props.indexOf('foo') === -1);
assert (props.length === 2)

// Test prototype chain
function Parent() {}
Parent.prototype.inheritedMethod = function() {};

function Child() {
  this[a] = 5;
  this[b] = function() {};
}
Child.prototype = new Parent;
Child.prototype.prototypeMethod = function() {};

props = Object.getOwnPropertySymbols (new Child());
// props should contain: a, b and the order is not defined!
assert (props.indexOf(a) !== -1);
assert (props.indexOf(b) !== -1);

assert (props.length === 2);

// Test non-emumerable symbols
var object = {};
var foo = Symbol ('foo');
var foo2 = Symbol ('foo2');
object[foo] = 'EnumerableSymbolProp';
Object.defineProperty(object, foo2, { value : 'NonEnumerableSymbolProp' });

props = Object.getOwnPropertySymbols (object);

assert (props.indexOf(foo) !== -1);
assert (props.indexOf(foo2) !== -1);
assert (props.length === 2);
assert (Object.getOwnPropertyDescriptor (object, foo).enumerable === true);
assert (Object.getOwnPropertyDescriptor (object, foo2).enumerable === false);

// Test non-object argument
try {
  Object.getOwnPropertySymbols ('hello');
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}
