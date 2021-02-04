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

checkSyntax ("var {a}");
checkSyntax ("var {a, o.a}");
checkSyntax ("var {a,,} = 4");
checkSyntax ("var {a :} = 4");
checkSyntax ("var {a : ,} = 4");
checkSyntax ("var {a : ['foobar']} = 4");
checkSyntax ("var {let}");
checkSyntax ("var {get = []");
checkSyntax ("var {get : 5}");
checkSyntax ("var {[a = {},}");
checkSyntax ("let {a,a} = []");
checkSyntax ("let {a : b, b} = []");
checkSyntax ("const {a,a} = []");
checkSyntax ("const {a : b, b} = []");
checkSyntax ("try { let {$} = $;");
checkSyntax ("let a, { 'x': a } = {x : 4};");
checkSyntax ("let a, { x: b.c } = {x : 6};");
checkSyntax ("let {a:(a)} = {a:1}");

mustThrow ("var {a} = null");
mustThrow ("var {a} = undefined");
mustThrow ("function f ({a : {}}) {}; f({});");
mustThrow ("function f ({}) {}; f();");

// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Destructuring_assignment

// Basic assignment
(function () {
  var o = {p: 42, q: true};
  var {p, q} = o;

  assert (p === 42);
  assert (q === true);
}) ();

// Assignment without declaration
(function () {
  var a, b;
  ({a, b} = {a: 1, b: 2});

  assert (a === 1);
  assert (b === 2);
}) ();

// Assigning to new variable names
(function () {
  var o = {p: 42, q: true};
  var {p: foo, q: bar} = o;

  assert (foo === 42);
  assert (bar === true);
}) ();

// Default values
(function () {
  var {a = 10, b = 5} = {a: 3};

  assert (a === 3);
  assert (b === 5);
}) ();


// Assigning to new variables names and providing default values
(function () {
  var {a: aa = 10, b: bb = 5} = {a: 3};

  assert (aa === 3);
  assert (bb === 5);
}) ();

// Nested object and array destructuring
(function () {
  const metadata = {
    title: 'Scratchpad',
    translations: [
      {
        locale: 'de',
        localization_tags: [],
        last_edit: '2014-04-14T08:43:37',
        url: '/de/docs/Tools/Scratchpad',
        title: 'JavaScript-Umgebung'
      }
    ],
    url: '/en-US/docs/Tools/Scratchpad'
  };

  let {
    title: englishTitle, // rename
    translations: [
      {
         title: localeTitle, // rename
      },
    ],
  } = metadata;

  assert (englishTitle === "Scratchpad");
  assert (localeTitle === "JavaScript-Umgebung");
}) ();

// Computed object property names and destructuring
(function () {
  let key = 'z';
  let {[key]: foo} = {z: 'bar'};

  assert (foo === "bar");
}) ();

// Invalid JavaScript identifier as a property name
(function () {
  const foo = { 'fizz-buzz': true };
  const { 'fizz-buzz': fizzBuzz } = foo;

  assert (fizzBuzz === true);
}) ();

// Combined Array and Object Destructuring
(function () {
  const props = [
    { id: 1, name: 'Fizz'},
    { id: 2, name: 'Buzz'},
    { id: 3, name: 'FizzBuzz'}
  ];

  const [,, { name }] = props;

  assert (name === "FizzBuzz");
}) ();

// The prototype chain is looked up when the object is deconstructed
(function () {
  var obj = {self: '123'};
  Object.getPrototypeOf(obj).prot = '456';
  const {self, prot} = obj;
  assert (self === '123');
  assert (prot === '456');
}) ();

// Test inner patterns I.
(function () {
  var a,b,c,d,e;
  var o = { a : { b: 2 }, c: 1, d: { e: undefined } };
  var { e: { b : a } = { b : 2, a : 1}, d: { e: { b : e = 2} = { b } } } = o;
  assert (a === 2);
  assert (b === undefined);
  assert (c === undefined);
  assert (d === undefined);
  assert (e === 2);
}) ();

// Test inner patterns II.
(function () {
  var a,b,c,d,e;
  var o = { a : [{ b : 2 ,}, d], e : 5 };

  var { a: [{b, c  = 3}, d = 4], e } = o;
  assert (a === undefined);
  assert (b === 2);
  assert (c === 3);
  assert (d === 4);
  assert (e === 5);
}) ();

// Multiple declaration
(function () {
  var {a} = {a : 1}, {b} = {b : 2};

  assert (a === 1);
  assert (b === 2);
}) ();

// Force the creation of lexical environment I.
(function () {
  const {a} = {a : 1};
  eval();

  assert (a === 1);
}) ();

// Force the creation of lexical environment II.
(function () {
  let {a} = {a : 1};
  eval();

  assert (a === 1);
}) ();

// Check the parsing of AssignmentElement
(function () {
  var a = 6;
  ({"a": ((a)) } = {a : 7});
  assert (a === 7);
}) ();

(function () {
  var o = {};

  ({ a : o.c } = { get a() { return 5.2 }})

  assert(o.c == 5.2);
}) ();

try {
  eval ("var a = 0; -{a} = {a:1}");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

var abruptObj = Object.defineProperty({}, 'a', {
  get() { throw 5.2; }
});

try {
  const { a } = abruptObj;
  assert (false);
} catch (e) {
  assert (e === 5.2);
}
