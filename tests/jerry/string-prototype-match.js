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

// check properties

function length_configurable()
{
  function is_es51() {
    return (typeof g === "function");
    { function g() {} }
  }
  return is_es51() ? false : true;
}

assert(Object.getOwnPropertyDescriptor(String.prototype.match, 'length').configurable === length_configurable());
assert(Object.getOwnPropertyDescriptor(String.prototype.match, 'length').enumerable === false);
assert(Object.getOwnPropertyDescriptor(String.prototype.match, 'length').writable === false);

function match_equals (match_result, expected)
{
  if (match_result.length !== expected.length)
  {
    return false;
  }

  for(var i = 0; i < expected.length; i++)
  {
    if (match_result[i] !== expected[i])
    {
      return false;
    }
  }

  return true;
}

assert (match_equals ("hello".match("o"), ["o"]));
assert ("hello".match(/ /g) == void 0);

assert (match_equals ("hello".match(/o/), ["o"]));

assert (match_equals ("hello".match(/l/), ["l"]));
assert (match_equals ("hello".match(/l/g), ["l", "l"]));

assert ("".match(/a/g) == void 0);

assert ("".match() !== void 0 );

assert (match_equals ("".match(), [""]));
assert (match_equals ("".match(undefined), [""]));
assert (match_equals ("".match(""), [""]));

assert (match_equals ("test 1, test 2, test 3, test 45".match(/[0-9]+/g), ["1", "2", "3", "45"]));

var re = new RegExp("", "g");
assert (match_equals ("a".match(re), ["", ""]));


/* Check Object coercible */
try {
  String.prototype.match.call(undefined, "");
  assert (false);
}
catch (e)
{
  assert (e instanceof TypeError);
}

/* Check toString conversion */
try {
  var obj = { toString: function() { throw new ReferenceError("foo"); } };
  String.prototype.match.call(obj, "");
  assert (false);
}
catch (e)
{
  assert (e instanceof ReferenceError);
  assert (e.message === "foo");
}

/* Check Invalid RegExp */
try {
  var obj = { toString: function() { throw new ReferenceError("foo"); } };
  "".match (obj);
  assert (false);
}
catch (e)
{
  assert (e instanceof ReferenceError);
  assert (e.message === "foo");
}

/* Check if lastIndex is set to 0 on start */
var re = /a/g;
re.lastIndex = 3;

assert (match_equals ("a".match(re), ["a"]));

class NewRegExp extends RegExp {
  [Symbol.match](str) {
      var result = RegExp.prototype[Symbol.match].call(this, str);
      var successful = 0;
      if (result) {
          successful = 1;
      }
      return successful;
  }
}

var str = 'This is a random string.';
var regexp = new NewRegExp(/[A-Z]/g);
assert(str.match(regexp) === 1);

try {
String.prototype.match.call(null, regexp);
assert(false);
} catch (e) {
assert(e instanceof TypeError);
}

var regexp2 = /[A-Z]/g;
regexp2[Symbol.match] = "foo";

try {
str.match(regexp2);
assert(false);
} catch (e) {
assert(e instanceof TypeError);
}

Object.defineProperty (regexp2, Symbol.match, { get () { throw 5 }});

try {
str.match(regexp2);
assert(false);
} catch (e) {
assert(e === 5);
}

var wrong_sytnax = "str.match(/[A-5]/g";

try {
eval(wrong_sytnax);
assert(false);
} catch (e) {
assert(e instanceof SyntaxError);
}

delete(RegExp.prototype[Symbol.match]);

try {
str.match(regexp);
assert(false);
} catch (e) {
assert(e instanceof TypeError);
}

var regexp3 = "foo";
RegExp.prototype[Symbol.match] = 3;

try {
str.match(regexp3);
assert(false);
} catch (e) {
assert(e instanceof TypeError);
}
