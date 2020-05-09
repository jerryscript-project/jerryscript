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

// tests for ECMA~262 v6 12.4.5 void UnaryExpression

var tests = [
  // IdentifierReference
  'var obj = {a : 10, ret : function(params) {void a = 42;}}',
  // NullLiteral
  'var a = null; void a = 42',
  // BooleanLiteral 
  'var a = true; void a = 42',
  'var a = false; void a = 42',
  // DecimalLiteral 
  'var a = 5; void a = 42',
  'var a = 1.23e4; void a = 42',
  // BinaryIntegerLiteral 
  'var a = 0b11; void a = 42',
  'var a = 0B11; void a = 42',
  // OctalIntegerLiteral 
  'var a = 0o66; void a = 42',
  'var a = 0O66; void a = 42',
  // HexIntegerLiteral 
  'var a = 0xFF; void a = 42',
  'var a = 0xFF; void a = 42',
  // StringLiteral
  'var a = "foo"; void a = 42',
  'var a = "\\n"; void a = 42',
  'var a = "\\uFFFF"; void a = 42',
  'var a ="\\u{F}"; void a = 42',
  // ArrayLiteral
  'var a = []; void a = 42',
  'var a = [1,a=5]; void a = 42',
  // ObjectLiteral
  'var a = {}; void a = 42',
  'var a = {"foo" : 5}; void a = 42',
  'var a = {5 : 5}; void a = 42',
  'var a = {a : 5}; void a = 42',
  'var a = {[key] : 5}; void a = 42',
  'var a = {func(){}}; void a = 42',
  'var a = {get(){}}; void a = 42',
  'var a = {set(prop){}}; void a = 42',
  'var a = {*func(){}}; void a = 42',
  // ClassExpression
  'class a {}; void a = 42',
  'class a {}; class b extends a {}; void b = 42',
  'class a {function(){}}; void a = 42',
  // GeneratorExpression
  'function *a (){}; void a = 42',
  // RegularExpressionLiteral
  'var a = /(?:)/; void a = 42',
  'var a = /a/; void a = 42',
  'var a = /[a]/; void a = 42',
  'var a = /a/g; void a = 42',
  // TemplateLiteral
  'var a = ``; void a = 42',
  'a = 5; var b = (`${a}`); void b = 42',
  'var a = `foo`; void a = 42',
  'var a = `\\uFFFF`; void a = 42',
  // MemberExpression
  'var a = [1,2,3]; void a[0] = 42',
  'var a = {0:12}; void a[0] = 42',
  'var a = {"foo":12}; void a.foo = 42',
  'var a = {func: function(){}}; void a.func = 42',
  // SuperProperty
  'class a {constructor() {Object.defineProperty(this, \'foo\', {configurable:true, writable:true, value:1}); }} ' +
  'class b extends a {constructor() {super();} foo() {void super.foo = 42;}}',
  // NewExpression
  'function a() {}; var b = new a(); void b = 42',
  'class g {constructor() {Object.defineProperty(this, \'foo\', {configurable:true, writable:true, value:1}); }}; ' +
  'var a = new g(); void a = 42',
  'class a {}; var n = new a(); void a = 42',
  // CallExpression
  'function a(prop){return prop}; var b = a(12); void b = 42',
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
    