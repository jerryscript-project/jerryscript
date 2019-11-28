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
