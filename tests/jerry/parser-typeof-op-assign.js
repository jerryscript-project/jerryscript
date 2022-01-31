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

// tests for ECMA~262 v6 12.4.5 typeof UnaryExpression

var tests = [
  // IdentifierReference
  'var obj = {a : 10, ret : function(params) {typeof a = 42;}}',
  // NullLiteral
  'var a = null; typeof a = 42',
  // BooleanLiteral 
  'var a = true; typeof a = 42',
  'var a = false; typeof a = 42',
  // DecimalLiteral 
  'var a = 5; typeof a = 42',
  'var a = 1.23e4; typeof a = 42',
  // BinaryIntegerLiteral 
  'var a = 0b11; typeof a = 42',
  'var a = 0B11; typeof a = 42',
  // OctalIntegerLiteral 
  'var a = 0o66; typeof a = 42',
  'var a = 0O66; typeof a = 42',
  // HexIntegerLiteral 
  'var a = 0xFF; typeof a = 42',
  'var a = 0xFF; typeof a = 42',
  // StringLiteral
  'var a = "foo"; typeof a = 42',
  'var a = "\\n"; typeof a = 42',
  'var a = "\\uFFFF"; typeof a = 42',
  'var a ="\\u{F}"; typeof a = 42',
  // ArrayLiteral
  'var a = []; typeof a = 42',
  'var a = [1,a=5]; typeof a = 42',
  // ObjectLiteral
  'var a = {}; typeof a = 42',
  'var a = {"foo" : 5}; typeof a = 42',
  'var a = {5 : 5}; typeof a = 42',
  'var a = {a : 5}; typeof a = 42',
  'var a = {[key] : 5}; typeof a = 42',
  'var a = {func(){}}; typeof a = 42',
  'var a = {get(){}}; typeof a = 42',
  'var a = {set(prop){}}; typeof a = 42',
  'var a = {*func(){}}; typeof a = 42',
  // ClassExpression
  'class a {}; typeof a = 42',
  'class a {}; class b extends a {}; typeof b = 42',
  'class a {function(){}}; typeof a = 42',
  // GeneratorExpression
  'function *a (){}; typeof a = 42',
  // RegularExpressionLiteral
  'var a = /(?:)/; typeof a = 42',
  'var a = /a/; typeof a = 42',
  'var a = /[a]/; typeof a = 42',
  'var a = /a/g; typeof a = 42',
  // TemplateLiteral
  'var a = ``; typeof a = 42',
  'a = 5; var b = (`${a}`); typeof b = 42',
  'var a = `foo`; typeof a = 42',
  'var a = `\\uFFFF`; typeof a = 42',
  // MemberExpression
  'var a = [1,2,3]; typeof a[0] = 42',
  'var a = {0:12}; typeof a[0] = 42',
  'var a = {"foo":12}; typeof a.foo = 42',
  'var a = {func: function(){}}; typeof a.func = 42',
  // SuperProperty
  'class a {constructor() {Object.defineProperty(this, \'foo\', {configurable:true, writable:true, value:1}); }} ' +
  'class b extends a {constructor() {super();} foo() {typeof super.foo = 42;}}',
  // NewExpression
  'function a() {}; var b = new a(); typeof b = 42',
  'class a {}; var n = new a(); typeof a = 42',
  'class g {constructor() {Object.defineProperty(this, \'foo\', {configurable:true, writable:true, value:1}); }}; ' +
  'var a = new g(); typeof a = 42',
  // CallExpression
  'function a(prop){return prop}; var b = a(12); typeof b = 42',
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
   