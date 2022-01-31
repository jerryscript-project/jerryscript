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

function must_throw (str) {
  try {
    eval ("switch (1) { default: " + str + "}");
    assert (false);
  }
  catch (e) {
  }

  try {
    eval (str);
    assert (false);
  }
  catch (e) {
  }
}

// strict functions created using the Function constructor
var strictF = new Function('"use strict"');

assert(strictF.hasOwnProperty('caller') === false);
assert(strictF.hasOwnProperty('arguments') === false);

must_throw("strictF.caller")
must_throw("strictF.arguments")
must_throw("strictF.caller = 1")
must_throw("strictF.arguments = 2")

// generator functions created using the Generator constructor
var GeneratorFunction = Object.getPrototypeOf(function*(){}).constructor
var generatorF = new GeneratorFunction()

assert(generatorF.hasOwnProperty('caller') === false);
assert(generatorF.hasOwnProperty('arguments') === false);

must_throw("generatorF.caller")
must_throw("generatorF.arguments")
must_throw("generatorF.caller = 1")
must_throw("generatorF.arguments = 2")

// async functions created using the AsyncFunction constructor
async function asyncF() {}

assert(asyncF.hasOwnProperty('caller') === false);
assert(asyncF.hasOwnProperty('arguments') === false);

must_throw("asyncF.caller")
must_throw("asyncF.arguments")
must_throw("asyncF.caller = 1")
must_throw("asyncF.arguments = 2")

// functions created using the bind method
var f = function () {};
var boundF = f.bind();

assert(boundF.hasOwnProperty('caller') === false);
assert(boundF.hasOwnProperty('arguments') === false);

must_throw("boundF.caller")
must_throw("boundF.arguments")
must_throw("boundF.caller = 1")
must_throw("boundF.arguments = 2")
