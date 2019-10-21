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

function NoParent () { }

Object.defineProperty (NoParent, Symbol.hasInstance, {
    value: function (arg) { return false; }
});

var obj = new NoParent ();

assert ((obj instanceof NoParent) === false);

try {
  Object.defineProperty (NoParent, Symbol.hasInstance, {
    value: function (arg) { return true; }
  });
  assert (false)
} catch (ex) {
  assert (ex instanceof TypeError);
}


function PositiveNumber () { }

Object.defineProperty (PositiveNumber, Symbol.hasInstance, {
    value: function (arg) { return (arg instanceof Number) && (arg >= 0); }
})

var num_a = new Number (33);
var num_b = new Number (-50);

assert ((num_a instanceof PositiveNumber) === true);
assert ((num_b instanceof PositiveNumber) === false);


function ErrorAlways () { }

Object.defineProperty (ErrorAlways, Symbol.hasInstance, {
    value: function (arg) { throw new URIError ("ErrorAlways"); }
})

try {
  (new Object ()) instanceof ErrorAlways;
  assert (false);
} catch (ex) {
  assert (ex instanceof URIError);
}


function NonCallable () { }

Object.defineProperty (NonCallable, Symbol.hasInstance, { value: 11 });

try {
  (new Object ()) instanceof NonCallable;
  assert (false);
} catch (ex) {
  assert (ex instanceof TypeError);
}


function ErrorGenerator () { }

Object.defineProperty (ErrorGenerator, Symbol.hasInstance, {
    get: function () { throw new URIError ("ErrorGenerator"); }
});

try {
  (new Object ()) instanceof ErrorGenerator;
  assert (false);
} catch (ex) {
  assert (ex instanceof URIError);
}
