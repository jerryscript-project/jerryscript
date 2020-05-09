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

// tests for ECMA-262 v6 12.2.1.5 and 12.2.10.3

var tests = [
  // this
  '(this) = 42;',
  // NullLiteral
  '(null) = 42;',
  // BooleanLiteral
  '(false) = 42;',
  '(true) = 42;',
  // DecimalLiteral
  '(12) = 42;',
  '(0.12) = 42;',
  '(.12) = 42;',
  '(1.23e4) = 42;',
  '(.23e-2) = 42;',
  // BinaryIntegerLiteral
  '(0b11) = 42; ',
  '(0B11) = 42;',
  // OctalIntegerLiteral
  '(0o67) = 42;',
  '(0O67) = 42;',
  // HexIntegerLiteral
  '(0xDD) = 42;',
  '(0XDD) = 42;',
  // StringLiteral
  '("foo") = 42;',
  '(\'foo\') = 42;',
  '("\\n") = 42;',
  '(\'\\n\') = 42;',
  '("\\0") = 42;',
  '(\'\\0\') = 42;',
  '("\\x0F") = 42;',
  '(\'\\x0F\') = 42;',
  '("\\uFFFF") = 42;',
  '(\'\\uFFFF\') = 42;',
  '("\\u{F}") = 42;',
  '(\'\\u{F}\') = 42;',
  '("\\d") = 42;',
  '(\'\\d\') = 42;',
  '("\\u000A") = 42;',
  '(\'\\u000A\') = 42;',
  // ArrayLiteral
  '([]) = 42;',
  '([1]) = 42;',
  '([1,,]) = 42;',
  '([1,a=5]) = 42;',
  // ObjectLiteral
  '({}) = 42;',
  '({yield}) = 42;',
  '({$a}) = 42;',
  '({a}) = 42;',
  '({a : 5}) = 42;',
  '({5 : 5}) = 42;',
  '({"foo" : 5}) = 42;',
  '({[key] : 5}) = 42;',
  '({func(){}}) = 42;',
  '({get(){}}) = 42;',
  '({set(prop){}}) = 42;',
  '({*func(){}}) = 42;',
  // FunctionExpression
  '(function(){}) = 42;',
  '(function(...a){}) = 42;',
  '(function a(){}) = 42;',
  '(function a(...a){}) = 42;',
  '(function (a){}) = 42;',
  '(function a(a){}) = 42;',
  // ClassExpression
  '(class a {}) = 42;',
  'class b {}; (class a extends b {}) = 42;',
  '(class a {function(){}}) = 42;',
  // GeneratorExpression
  '(function *a (){}) = 42;',
  // RegularExpressionLiteral
  '(/(?:)/) = 42;',
  '(/a/) = 42;',
  '(/\\a/) = 42;',
  '(/[a]/) = 42;',
  '(/a/g) = 42;',
  '(/a/i) = 42;',
  '(/a/m) = 42;',
  '(/a/u) = 42;',
  '(/a/y) = 42;',
  // TemplateLiteral
  '(``) = 42;',
  'a = 5; (`${a}`) = 42;',
  '(`\\a`) = 42;',
  '(`\\0`) = 42;',
  '(`\\xFF`) = 42;',
  '(`\\uFFFF`) = 42;',
  '(`\\u{F}`) = 42;',
  '(`foo`) = 42;',
  '(`\\u000A`) = 42;',
];

for (var i = 0; i < tests.length; i++)
{
  try {
    eval(tests[i]);
    assert(false);
  } catch (e) {
    assert(e instanceof SyntaxError);
  }
}
