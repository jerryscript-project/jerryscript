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

assert ("abcabc".replace("bc", ":") === "a:abc");
assert ("hello".replace("", ":") === ":hello");

assert ("xabcxabcx".replace (/abc/g, "[$&][$`][$']") === "x[abc][x][xabcx]x[abc][xabcx][x]x");
assert ("abc".replace (/a(b)c|d()/, "[$1][$01][$2][$02][$99][$123][$012]") === "[b][b][][][$99][b23][b2]");
assert ("abc".replace("abc", "$x$$5$0$00$") === "$x$5$0$00$");

assert ("#x#".replace("x", "$1") === "#$1#");
assert ("#x#".replace(/(x)/, "$1$2") === "#x$2#");
assert ("#x#".replace(/(x)/, "$01$02$11$20") === "#x$02x1$20#");
assert ("#xy#".replace(/(x)((((((((((y))))))))))/, "$07|$20|$11|$12|$110|$99|$011") === "#y|y0|y|x2|y0|y9|x1#");
assert ("#xy#".replace(/(x)((((((((y))))))))/, "$00|$01|$011|$090|$10|$99") === "#$00|x|x1|y0|x0|y9#");

assert ("a true true story".replace(true) === "a undefined true story");
assert ("1234".replace(23, 32) === "1324");

assert ("abcabc".replace(/bc/, ":") === "a:abc");
assert ("axbcxx".replace(/x*/g, ":") === ":a::b:c::");

assert ("".replace(/|/g,"஻") === "஻");
assert ("஻BB8B@abXde^".replace(/a/g,"$஻Bce((/a%") === "஻BB8B@$஻Bce((/a%bXde^");
assert ("abcab".replace(/a/g,"˙Ł$Đ") === "˙Ł$Đbc˙Ł$Đb");
assert ("˙Ł$Đbc˙Ł$Đb".replace("Ł$","ab") === "˙abĐbc˙Ł$Đb");

assert (String.prototype.replace.call (12321, /2/g, ".") === "1.3.1");

try
{
  String.prototype.replace.call (null, "u", ".");
  assert (false);
}
catch (e)
{
  assert (e instanceof TypeError);
}

assert ("98765".replace(76, function () { return {}; }) === "98[object Object]5");

function concat_arguments()
{
  var str = "";
  for (var i = 0; i < arguments.length; i++)
  {
    str += "[" + arguments[i] + "]";
  }
  return str;
}

assert ("abcdabcd".replace("cd", concat_arguments) === "ab[cd][2][abcdabcd]abcd");
assert ("abcdef".replace (/a((b)c)|d()/, concat_arguments) === "[abc][bc][b][undefined][0][abcdef]def");

try
{
  "x".replace("x", function() { throw "MyError"; });
  assert (false);
}
catch (e)
{
  assert (e === "MyError");
}

assert ("\ud801\udc00".replace("\ud801", "#") === "#\udc00");
assert ("\ud801\udc00".replace("\udc00", "#") === "\ud801#");

var global = this;

function case1()
{
  assert(this === global);
  return "y";
}

function case2()
{
  "use strict";
  assert(this === undefined);
  return "y";
}

assert ("x".replace("x", case1) === "y");
assert ("x".replace("x", case2) === "y");

var regexp = /r/g;

Object.defineProperty(regexp, "lastIndex", {
  configurable : false,
  enumerable : false,
  value : 0,
  writable : false
});

try {
  "r".replace (regexp, "x");
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

// The real "exec" never returns with a number.
Object.getPrototypeOf(/x/).exec = function () { return 1234; }

assert (/y/.exec("y") === 1234);

// Changing exec should not affect replace.
assert ("y".replace (/y/, "x") === "x");
