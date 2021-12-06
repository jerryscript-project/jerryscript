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

function assertArrayEqual (actual, expected) {
  assert (actual.length === expected.length);

  for (var i = 0; i < actual.length; i++) {
    assert (actual[i] === expected[i]);
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

function mustNotThrow (str) {
  try {
    eval (str);
  } catch (e) {
    assert (false);
  }
}

checkSyntax ("var [a]");
checkSyntax ("var [a, o.a]");
checkSyntax ("var [a, ...b,]");
checkSyntax ("var [a, ...b = 4]");
checkSyntax ("var [a, ...[b] = 4]");
checkSyntax ("var [let]");
checkSyntax ("var [get = []");
checkSyntax ("var [get : 5]");
checkSyntax ("var [[a = {},]");
checkSyntax ("let [a,a] = []");
checkSyntax ("let [a, ...a] = []");
checkSyntax ("const [a,a] = []");
checkSyntax ("const [a, ...a] = []");
checkSyntax ("[new Object()] = []");
checkSyntax ("[Object()] = []");
checkSyntax ("[(a, b, d, c)] = []");
checkSyntax ("[super] = []");
checkSyntax ("[this] = []");
checkSyntax ("[()] = []");
checkSyntax ("try { let [$] = $;");
checkSyntax ("let a, [ b.c ] = [6];");
checkSyntax ("let [(a)] = [1]");
checkSyntax ("[...a = []] = [1]");
checkSyntax ("[...[a] = []] = [1]");
checkSyntax ("[...[a, [...b] = []] = []] = [1]");

mustThrow ("var [a] = 4");
mustThrow ("var [a] = 5");
mustThrow ("var [a] = {}");
mustThrow ("var [a] = { get [Symbol.iterator] () { throw new TypeError } }");
mustThrow ("var [a] = { [Symbol.iterator] () {} }");
mustThrow ("var [a] = { [Symbol.iterator] () { return {} } }");
mustThrow ("var [a] = { [Symbol.iterator] () { return { next: 5 } } }");
mustThrow ("var [a] = { [Symbol.iterator] () { return { next: 5 } } }");
mustThrow ("var [a] = { [Symbol.iterator] () { return { get next() { throw new TypeError } } } }");
mustThrow ("var [a] = { [Symbol.iterator] () { return { next () { } } } }");
mustThrow ("var [a] = { [Symbol.iterator] () { return { next () { } } } }");
mustThrow ("var [a] = { [Symbol.iterator] () { return { next () { return { get value () { throw new TypeError }}}}}}");
mustThrow ("var [a] = { [Symbol.iterator] () { return { next () { return { get done () { throw new TypeError }}}}}}");

// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Destructuring_assignment

// Basic variable assignment
(function () {
  var foo = ["one", "two", "three"];

  var [red, yellow, green] = foo;
  assert (red === "one");
  assert (yellow === "two");
  assert (green === "three");
}) ();

// Assignment separate from declaration
(function () {
  var a, b;

  [a, b] = [1, 2];
  assert (a === 1);
  assert (b === 2);
}) ();

// Default values
(function () {
  var a, b;
  [a = 5, b = 7] = [1];

  assert (a === 1);
  assert (b === 7);
}) ();

// Swapping variables
(function () {
  var a = 1;
  var b = 3;

  [a, b] = [b, a];
  assert (a === 3);
  assert (b === 1);

  var arr = [1,2,3];
  [arr[2], arr[1]] = [arr[1], arr[2]];
  assertArrayEqual (arr, [1, 3, 2]);
}) ();

// Parsing an array returned from a function
(function () {
  function f() {
    return [1, 2];
  }

  var a, b;
  [a, b] = f();
  assert (a === 1);
  assert (b === 2);
}) ();

// Ignoring some returned values
(function () {
  function f() {
    return [1, 2, 3];
  }

  var a, b;
  [a, ,b] = f();
  assert (a === 1);
  assert (b === 3);
}) ();

// Ignoring some returned values
(function () {
  var [a, ...b] = [1, 2, 3];
  assert (a === 1);
  assertArrayEqual (b, [2, 3]);
}) ();

// Unpacking values from a regular expression match
(function () {
  function parseProtocol(url) {
    var parsedURL = /^(\w+)\:\/\/([^\/]+)\/(.*)$/.exec(url);
    if (!parsedURL) {
      return false;
    }

    var [, protocol, fullhost, fullpath] = parsedURL;
    return protocol;
  }

  assert (parseProtocol("https://developer.mozilla.org/en-US/Web/JavaScript") === "https");
}) ();

// Test inner patterns I.
(function () {
  let [a, [b, [c = 4, d = 5]], [e] = [6]] = [1, [2, [3,undefined]]];

  assert (a === 1);
  assert (b === 2);
  assert (c === 3);
  assert (d === 5);
  assert (e === 6);
}) ();

// Test inner patterns II.
(function () {
  var o = {};
  [a, b, c, o.a = 4, o.b, o.c = 3] = ["1", "2", "3", undefined, "8", "6"];

  assert (a === "1");
  assert (b === "2");
  assert (c === "3");
  assert (o.a === 4);
  assert (o.b === "8");
  assert (o.c === "6");
}) ();

// Test rest element I.
(function () {
  var o = {};
  [...o.a] = ["1", "2", "3"];

  assertArrayEqual (o.a, ["1", "2", "3"]);
}) ();

// Test rest element II.
(function () {
  [...[a,b,c]] = ["1", "2", "3"];

  assert (a === "1");
  assert (b === "2");
  assert (c === "3");
}) ();

// Test inner object pattern I.
(function () {
  [{f : a, g : b}, , , ...[c, d, e]] = [{ f : "1", g : "2"}, 3, 4, 5, 6, 7];

  assert (a === "1");
  assert (b === "2");
  assert (c === 5);
  assert (d === 6);
  assert (e === 7);
}) ();

// Multiple declaration
(function () {
  var [a] = [1], [b] = [2];

  assert (a === 1);
  assert (b === 2);
}) ();

// Force the creation of lexical environment I.
(function () {
  const [a] = [1];
  eval();

  assert (a === 1);
}) ();

// Force the creation of lexical environment II.
(function () {
  let [a] = [1];
  eval();

  assert (a === 1);
}) ();

// Check the parsing of AssignmentElement
(function () {
  var a = 6;
  [((a))] = [7];
  assert (a === 7);
}) ();

// Test iterator closing
function __createIterableObject (arr, methods) {
  methods = methods || {};
  if (typeof Symbol !== 'function' || !Symbol.iterator) {
    return {};
  }
  arr.length++;
  var iterator = {
    next: function() {
      return { value: arr.shift(), done: arr.length <= 0 };
    },
    'return': methods['return'],
    'throw': methods['throw']
  };
  var iterable = {};
  iterable[Symbol.iterator] = function () { return iterator; };
  return iterable;
};

(function () {
  var closed = false;
  var iter = __createIterableObject([1, 2, 3], {
    'return': function() { closed = true; return {}; }
  });
  var [a, b] = iter;
  assert (closed === true);
  assert (a === 1);
  assert (b === 2);
}) ();

(function () {
  var value = { y: "42" };
  var x = {};
  var assignmentResult, iterationResult, iter;

  iter = (function*() {
    assignmentResult = { y: x[yield] } = value;
  }());

  iterationResult = iter.next();

  assert (assignmentResult === undefined);
  assert (iterationResult.value === undefined);
  assert (iterationResult.done === false);
  assert (x.prop === undefined);

  iterationResult = iter.next('prop');

  assert (assignmentResult === value);
  assert (iterationResult.value === undefined);
  assert (iterationResult.done === true);
  assert (x.prop === "42");
}) ();

(function () {
  var value = { foo: "42" };
  var x = {};
  var assignmentResult, iterationResult, iter;

  iter = (function*() {
    assignmentResult = { ['f' + 'o' + 'o']: x[yield] } = value;
  }());

  iterationResult = iter.next();

  assert (assignmentResult === undefined);
  assert (iterationResult.value === undefined);
  assert (iterationResult.done === false);
  assert (x.prop === undefined);

  iterationResult = iter.next('prop');

  assert (assignmentResult === value);
  assert (iterationResult.value === undefined);
  assert (iterationResult.done === true);
  assert (x.prop === "42");
}) ();

mustThrow (`var iter = __createIterableObject([],
           { get 'return'() { throw new TypeError() }});
           var [a] = iter`);

mustNotThrow (`var iter = __createIterableObject([],
              { 'return': 5 });
              var [a] = iter`);

mustNotThrow (`var iter = __createIterableObject([],
              { 'return': function() { return 5; }});
              var [a] = iter`);

mustThrow (`try { throw 5 } catch (e) {
            var iter = __createIterableObject([],
            { get 'return'() { throw new TypeError() }});
            var [a] = iter }`);

mustNotThrow (`try { throw 5 } catch (e) {
              var iter = __createIterableObject([],
              { 'return': 5 });
              var [a] = iter }`);

mustNotThrow (`try { throw 5 } catch (e) {
              var iter = __createIterableObject([],
              { 'return': function() { return 5; }});
              var [a] = iter }`);

try {
  eval ("var a = 0; 1 + [a] = [1]");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}
