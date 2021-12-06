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

// tests for ECMA-262 v6 12.4.5 + UnaryExpression and - UnaryExpression

var tests = [
  // IdentifierReference
  'var obj = {a : 10, ret : function(params) {return +a = 42;}}',
  'var obj = {a : 10, ret : function(params) {return -a = 42;}}',
  // NullLiteral
  'var a = null; +a = 42',
  'var a = null; -a = 42',
  // BooleanLiteral 
  'var a = true; +a = 42',
  'var a = false; +a = 42',
  'var a = true; -a = 42',
  'var a = false; -a = 42',
  // DecimalLiteral 
  'var a = 5; +a = 42',
  'var a = 1.23e4; +a = 42',
  'var a = 5; -a = 42',
  'var a = 1.23e4; -a = 42',
  // BinaryIntegerLiteral 
  'var a = 0b11; +a = 42',
  'var a = 0B11; +a = 42',
  'var a = 0b11; -a = 42',
  'var a = 0B11; -a = 42',
  // OctalIntegerLiteral 
  'var a = 0o66; +a = 42',
  'var a = 0O66; +a = 42',
  'var a = 0o66; -a = 42',
  'var a = 0O66; -a = 42',
  // HexIntegerLiteral 
  'var a = 0xFF; +a = 42',
  'var a = 0xFF; +a = 42',
  'var a = 0xFF; -a = 42',
  'var a = 0xFF; -a = 42',
  // StringLiteral
  'var a = "foo"; +a = 42',
  'var a = "\\n"; +a = 42',
  'var a = "\\uFFFF"; +a = 42',
  'var a ="\\u{F}"; +a = 42',
  'var a = "foo"; -a = 42',
  'var a = "\\n"; -a = 42',
  'var a = "\\uFFFF"; -a = 42',
  'var a ="\\u{F}"; -a = 42',
  // ArrayLiteral
  'var a = []; +a = 42',
  'var a = [1,a=5]; +a = 42',
  'var a = []; -a = 42',
  'var a = [1,a=5]; -a = 42',
  // ObjectLiteral
  'var a = {}; +a = 42',
  'var a = {"foo" : 5}; +a = 42',
  'var a = {5 : 5}; +a = 42',
  'var a = {a : 5}; +a = 42',
  'var a = {[key] : 5}; +a = 42',
  'var a = {func(){}}; +a = 42',
  'var a = {get(){}}; +a = 42',
  'var a = {set(prop){}}; +a = 42',
  'var a = {*func(){}}; +a = 42',
  'var a = {}; -a = 42',
  'var a = {"foo" : 5}; -a = 42',
  'var a = {5 : 5}; -a = 42',
  'var a = {a : 5}; -a = 42',
  'var a = {[key] : 5}; -a = 42',
  'var a = {func(){}}; -a = 42',
  'var a = {get(){}}; -a = 42',
  'var a = {set(prop){}}; -a = 42',
  'var a = {*func(){}}; -a = 42',
  // ClassExpression
  'class a {}; +a = 42',
  'class a {}; class b extends a {}; +b = 42',
  'class a {function(){}}; +a = 42',
  'class a {}; -a = 42',
  'class a {}; class b extends a {}; -b = 42',
  'class a {function(){}}; -a = 42',
  // GeneratorExpression
  'function *a (){}; +a = 42',
  'function *a (){}; -a = 42',
  // RegularExpressionLiteral
  'var a = /(?:)/; +a = 42',
  'var a = /a/; +a = 42',
  'var a = /[a]/; +a = 42',
  'var a = /a/g; +a = 42',
  'var a = /(?:)/; -a = 42',
  'var a = /a/; -a = 42',
  'var a = /[a]/; -a = 42',
  'var a = /a/g; -a = 42',
  // TemplateLiteral
  'var a = ``; +a = 42',
  'a = 5; var b = (`${a}`); +b = 42',
  'var a = `foo`; +a = 42',
  'var a = `\\uFFFF`; +a = 42',
  'var a = ``; -a = 42',
  'a = 5; var b = (`${a}`); -b = 42',
  'var a = `foo`; -a = 42',
  'var a = `\\uFFFF`; -a = 42',
  // MemberExpression
  'var a = [1,2,3]; +a[0] = 42',
  'var a = {0:12}; +a[0] = 42',
  'var a = {"foo":12}; +a.foo = 42',
  'var a = {func: function(){}}; +a.func = 42',
  'var a = [1,2,3]; -a[0] = 42',
  'var a = {0:12}; -a[0] = 42',
  'var a = {"foo":12}; -a.foo = 42',
  'var a = {func: function(){}}; -a.func = 42',
  // SuperProperty
  'class a {constructor() {Object.defineProperty(this, \'foo\', {configurable:true, writable:true, value:1}); }} ' +
  'class b extends a {constructor() {super();} foo() {+super.foo = 42;}}',
  'class a {constructor() {Object.defineProperty(this, \'foo\', {configurable:true, writable:true, value:1}); }} ' +
  'class b extends a {constructor() {super();} foo() {-super.foo = 42;}}',
  // NewExpression
  'function a() {}; var b = new a(); +b = 42',
  'function a() {}; var b = new a(); -b = 42',
  'class g {constructor() {Object.defineProperty(this, \'foo\', {configurable:true, writable:true, value:1}); }}; ' +
  'var a = new g(); +a = 42',
  'class g {constructor() {Object.defineProperty(this, \'foo\', {configurable:true, writable:true, value:1}); }}; ' +
  'var a = new g(); -a = 42',
  'class a {}; var n = new a(); +a = 42',
  'class a {}; var n = new a(); -a = 42',
  // CallExpression
  'function a(prop){return prop}; var b = a(12); +b = 42',
  'function a(prop){return prop}; var b = a(12); -b = 42',
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
  