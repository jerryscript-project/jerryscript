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

var regexp = /[0-9]+/g;
var str = '2016-01-02';
var num = 12131233;

// Test with string input
var result = regexp[Symbol.match](str);
assert(result.toString() === "2016,01,02");

regexp = /[0-5]+/g;
result = regexp[Symbol.match](str);
assert(result.toString() === "201,01,02");

regexp = /[0-1]+/g;
result = regexp[Symbol.match](str);
assert(result.toString() === "01,01,0");

regexp = /([0-9]+)-([0-9]+)-([0-9]+)/g
result = regexp[Symbol.match](str);
assert(result.toString() === "2016-01-02");

// Test with number input
regexp = /[0-9]+/g;
result = regexp[Symbol.match](num);
assert(result.toString() === "12131233");

// Test with empty string
result = regexp[Symbol.match]('');
assert(result === null);

// Test with undefined
result = regexp[Symbol.match](undefined);
assert(result === null);

// Test when input is not a regexp
regexp = 12;

try {
  result = regexp[Symbol.match](str);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// Test with RegExp subclass where we override the [Symbol.match] function
class RegExpSub extends RegExp {
  [Symbol.match](str) {
    var result = RegExp.prototype[Symbol.match].call(this, str);
    if (result) {
      return "VALID";
    }
    else
    {
      return "INVALID";
    }
  }
}

var regexp1 = new RegExpSub('([0-9]+)-([0-9]+)-([0-9]+)');
result = regexp1[Symbol.match](str);
assert(result === "VALID");

var o = {
  lastIndex: 0,
  global: true,
  exec: function () {
    if (this.lastIndex === 0)
    {
      this.lastIndex = 1;
      return {0: 3.14, index: 2};
    }

    return null;
  }
}

var result = RegExp.prototype[Symbol.match].call(o, "asd");
assert(result.length === 1);
assert(typeof result[0] === "string");
assert(result[0] === "3.14");
