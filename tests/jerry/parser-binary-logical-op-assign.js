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

// tests for ECMA~262 v6 12.12.1

var tests = [
  // this
  'this && this = 42',
  'this && null = 42',
  'this && undefined = 42',
  'this && true = 42',
  'this && 12 = 42',
  'this && "foo" = 42',
  'this && [12] = 42',
  'this && class a {} = 42',
  'this && function a(){} = 42',
  'this && /[a]/ = 42',
  'this && `foo` = 42',
  'this || this = 42',
  'this || null = 42',
  'this || undefined = 42',
  'this || true = 42',
  'this || 12 = 42',
  'this || "foo" = 42',
  'this || [12] = 42',
  'this || class a {} = 42',
  'this || function a(){} = 42',
  'this || /[a]/ = 42',
  'this || `foo` = 42',
  // undefined
  'undefined && null = 42',
  'undefined && undefined = 42',
  'undefined && true = 42',
  'undefined && 12 = 42',
  'undefined && "foo" = 42',
  'undefined && [12] = 42',
  'undefined && class a {} = 42',
  'undefined && function a(){} = 42',
  'undefined && /[a]/ = 42',
  'undefined && `foo` = 42',
  'undefined || null = 42',
  'undefined || undefined = 42',
  'undefined || true = 42',
  'undefined || 12 = 42',
  'undefined || "foo" = 42',
  'undefined || [12] = 42',
  'undefined || class a {} = 42',
  'undefined || function a(){} = 42',
  'undefined || /[a]/ = 42',
  'undefined || `foo` = 42',
  // NullLiteral
  'null && null = 42',
  'null && true = 42',
  'null && 12 = 42',
  'null && "foo" = 42',
  'null && [12] = 42',
  'null && class a {} = 42',
  'null && function a(){} = 42',
  'null && /[a]/ = 42',
  'null && `foo` = 42',
  'null || null = 42',
  'null || true = 42',
  'null || 12 = 42',
  'null || "foo" = 42',
  'null || [12] = 42',
  'null || class a {} = 42',
  'null || function a(){} = 42',
  'null || /[a]/ = 42',
  'null || `foo` = 42',
  // BooleanLiteral 
  'true && true = 42',
  'true && 12 = 42',
  'true && "foo" = 42',
  'true && [12] = 42',
  'true && class a {} = 42',
  'true && function a(){} = 42',
  'true && /[a]/ = 42',
  'true && `foo` = 42',
  'true || true = 42',
  'true || 12 = 42',
  'true || "foo" = 42',
  'true || [12] = 42',
  'true || class a {} = 42',
  'true || function a(){} = 42',
  'true || /[a]/ = 42',
  'true || `foo` = 42',
  // DecimalLiteral 
  '5 && 12 = 42',
  '5 && "foo" = 42',
  '5 && [12] = 42',
  '5 && class a {} = 42',
  '5 && function a(){} = 42',
  '5 && /[a]/ = 42',
  '5 && `foo` = 42',
  '5 || 12 = 42',
  '5 || "foo" = 42',
  '5 || [12] = 42',
  '5 || class a {} = 42',
  '5 || function a(){} = 42',
  '5 || /[a]/ = 42',
  '5 || `foo` = 42',
  // StringLiteral
  '"foo" && "foo" = 42',
  '"foo" && [12] = 42',
  '"foo" && class a {} = 42',
  '"foo" && function a(){} = 42',
  '"foo" && /[a]/ = 42',
  '"foo" && `foo` = 42',
  '"foo" || "foo" = 42',
  '"foo" || [12] = 42',
  '"foo" || class a {} = 42',
  '"foo" || function a(){} = 42',
  '"foo" || /[a]/ = 42',
  '"foo" || `foo` = 42',
  // ArrayLiteral
  '[12] && [12] = 42',
  '[12] && class a {} = 42',
  '[12] && function a(){} = 42',
  '[12] && /[a]/ = 42',
  '[12] && `foo` = 42',
  '[12] || [12] = 42',
  '[12] || class a {} = 42',
  '[12] || function a(){} = 42',
  '[12] || /[a]/ = 42',
  '[12] || `foo` = 42',
  // ObjectLiteral
  'this && {} = 42',
  'this || {} = 42',
  'undefined && {} = 42',
  'undefined || {} = 42',
  'null && {} = 42',
  'null || {} = 42',
  'true && {} = 42',
  'true || {} = 42',
  '5 && {} = 42',
  '5 || {} = 42',
  '"foo" && {} = 42',
  '"foo" || {} = 42',
  '[12] && {} = 42',
  '[12] || {} = 42',
  '/[a]/ && {} = 42',
  '`foo` && {} = 42',
  '/[a]/ || {} = 42',
  '`foo` || {} = 42',
  // RegularExpressionLiteral
  '/[a]/ && class a{} = 42',
  '/[a]/ && function a(){} = 42',
  '/[a]/ && /[a]/ = 42',
  '/[a]/ && `foo` = 42',
  '/[a]/ || class a{} = 42',
  '/[a]/ || function a(){} = 42',
  '/[a]/ || /[a]/ = 42',
  '/[a]/ || `foo` = 42',
  // TemplateLiteral
  '`foo` && class a{} = 42',
  '`foo` && function a(){} = 42',
  '`foo` && `foo` = 42',
  '`foo` || class a{} = 42',
  '`foo` || function a(){} = 42',
  '`foo` || `foo` = 42',
  // combining with BitwiseORExpression and LogicalANDExpression
  '[] << !5 * function a(){} | "foo" == !"foo" % 6 + !4 / 7 || 23 & +[] >> 4 * delete 12 = 42',
  '[] << !5 * function a(){} | "foo" == !"foo" % 6 + !4 / 7 && 23 & +[] >> 4 * delete 12 = 42',
  '4 % [1,2] - class a{} << typeof "foo" == !25 | [] != 13 >> "foo" + delete 3 / /(?:)/ || ++a * "foo" + function a(){} = 42',
  '4 % [1,2] - class a{} << typeof "foo" == !25 | [] != 13 >> "foo" + delete 3 / /(?:)/ && ++a * "foo" + function a(){} = 42',
  '`foo` === "foo" | +/(?:)/ % 25 + void 6 % delete class a {} || 5 >> void function a(){} !== +6 * !6 - ~4 / "foo" = 42',
  '`foo` === "foo" | +/(?:)/ % 25 + void 6 % delete class a {} && 5 >> void function a(){} !== +6 * !6 - ~4 / "foo" = 42',
  'var a = 5; 5 + class a{} << typeof "foo" == !25 | [] != 13 >> "foo" + delete 3 || 5 << [] * 12 != [1,2,3] ^ ++a  * typeof "foo" ^ function a(){} = 42',
  'var a = 5; 5 + class a{} << typeof "foo" == !25 | [] != 13 >> "foo" + delete 3 && 5 << [] * 12 != [1,2,3] ^ ++a  * typeof "foo" ^ function a(){} = 42',
  '[1,2] >> +5 * function a(){} ^ !"foo" == !"foo" % ~6 + !4 / void /foo/ || [3] << !5 * function a(){} ^ "foo" == !"foo" % 6 + !0b11 / 0xFF = 42',
  '[1,2] >> +5 * function a(){} ^ !"foo" == !"foo" % ~6 + !4 / void /foo/ && [3] << !5 * function a(){} ^ "foo" == !"foo" % 6 + !0b11 / 0xFF = 42',
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
