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

function check_syntax_error(code)
{
  try {
    eval(code)
    assert(false)
  } catch (e) {
    assert(e instanceof SyntaxError)
  }
}

check_syntax_error("#a");
check_syntax_error("class A { #a; #a; }");
check_syntax_error("class A { # a; }");
check_syntax_error("class A { #5; }");
check_syntax_error("class A { #\"bar\"; }");
check_syntax_error("class A { ##; }");
check_syntax_error("class A { ##a; }");
check_syntax_error("class A { #a; foo(){ delete this.#a } }");
check_syntax_error("class A { #constructor; }");
check_syntax_error("class A { #constructor(){}; }");
check_syntax_error("class A { #a; foo(){ if(#a){ } } }");
check_syntax_error("class A { #a; foo(){ if(#b){ } } }");
check_syntax_error("class A { #a; foo(){ if(#a in this){ let b = #b } } }");
check_syntax_error("class A { #a; }; let b = new A(); b.#a");
check_syntax_error("class A { #a; }; class B extends A { foo(){ return this.#a; } }");
check_syntax_error("class A { #a; get #a(){}; }");
check_syntax_error("class A { set #a(){}; get #a(){}; #a; }");
check_syntax_error("class A { get #a(){}; get #a(){}; }");
check_syntax_error("class A { async *#a(){}; #a }");
check_syntax_error("class A { async get #a(){}; }");
check_syntax_error("class A { get *#a(){}; }");
check_syntax_error("class A { static get #a(){}; set #a(){}; #a; }");
check_syntax_error("class A { static #a(){}; #a; }");
check_syntax_error("class A extends B{ foo(){ super.#a }}");
check_syntax_error("class A extends function() { x = this.#foo; }{ #foo; };");

// TODO: add runtime tests
