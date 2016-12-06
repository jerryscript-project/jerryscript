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

var e;

/* Error */
e = new Error ();
assert (e.name === "Error");
assert (e.message === "");
assert (e.toString() === "Error");

e = new Error("some message");
assert (e.name === "Error");
assert (e.message === "some message");
assert (e.toString() === "Error: some message");

e.name = "";
assert (e.toString() === "some message");
e.message = "";
assert (e.toString() === "");

assert (Error.prototype.toString !== Object.prototype.toString);
assert (Error.prototype.constructor === Error);
assert (Error.prototype.name === "Error");
assert (Error.prototype.message === "");
assert (Error.prototype.toString() === "Error");

/* TypeError */
e = new TypeError ();
assert (e.name === "TypeError");
assert (e.message === "");
assert (e.toString() === "TypeError");

e = new TypeError("some message");
assert (e.name === "TypeError");
assert (e.message === "some message");
assert (e.toString() === "TypeError: some message");

e.name = "";
assert (e.toString() === "some message");
e.message = "";
assert (e.toString() === "");

assert (TypeError.prototype.toString === Error.prototype.toString);
assert (TypeError.prototype.constructor === TypeError);
assert (TypeError.prototype.name === "TypeError");
assert (TypeError.prototype.message === "");
assert (TypeError.prototype.toString() === "TypeError");

try
{
  null[1] = 'abcd';

  assert (false);
}
catch (e)
{
  assert(e instanceof TypeError);
  assert(e instanceof Error);
  assert(e instanceof Object);

  assert(!(e instanceof Function));
}


/* ReferenceError */
e = new ReferenceError ();
assert (e.name === "ReferenceError");
assert (e.message === "");
assert (e.toString() === "ReferenceError");

e = new ReferenceError("some message");
assert (e.name === "ReferenceError");
assert (e.message === "some message");
assert (e.toString() === "ReferenceError: some message");

e.name = "";
assert (e.toString() === "some message");
e.message = "";
assert (e.toString() === "");

assert (ReferenceError.prototype.toString === Error.prototype.toString);
assert (ReferenceError.prototype.constructor === ReferenceError);
assert (ReferenceError.prototype.name === "ReferenceError");
assert (ReferenceError.prototype.message === "");
assert (ReferenceError.prototype.toString() === "ReferenceError");

try
{
  var a = non_existing_variable;

  assert (false);
}
catch (e)
{
  assert(e instanceof ReferenceError);
  assert(e instanceof Error);
  assert(e instanceof Object);

  assert(!(e instanceof Function));
}

/* EvalError */
e = new EvalError ();
assert (e.name === "EvalError");
assert (e.message === "");
assert (e.toString() === "EvalError");

e = new EvalError("some message");
assert (e.name === "EvalError");
assert (e.message === "some message");
assert (e.toString() === "EvalError: some message");

e.name = "";
assert (e.toString() === "some message");
e.message = "";
assert (e.toString() === "");

assert (EvalError.prototype.toString === Error.prototype.toString);
assert (EvalError.prototype.constructor === EvalError);
assert (EvalError.prototype.name === "EvalError");
assert (EvalError.prototype.message === "");
assert (EvalError.prototype.toString() === "EvalError");

/* RangeError */
e = new RangeError ();
assert (e.name === "RangeError");
assert (e.message === "");
assert (e.toString() === "RangeError");

e = new RangeError("some message");
assert (e.name === "RangeError");
assert (e.message === "some message");
assert (e.toString() === "RangeError: some message");

e.name = "";
assert (e.toString() === "some message");
e.message = "";
assert (e.toString() === "");

assert (RangeError.prototype.toString === Error.prototype.toString);
assert (RangeError.prototype.constructor === RangeError);
assert (RangeError.prototype.name === "RangeError");
assert (RangeError.prototype.message === "");
assert (RangeError.prototype.toString() === "RangeError");

/* SyntaxError */
e = new SyntaxError ();
assert (e.name === "SyntaxError");
assert (e.message === "");
assert (e.toString() === "SyntaxError");

e = new SyntaxError("some message");
assert (e.name === "SyntaxError");
assert (e.message === "some message");
assert (e.toString() === "SyntaxError: some message");

e.name = "";
assert (e.toString() === "some message");
e.message = "";
assert (e.toString() === "");

assert (SyntaxError.prototype.toString === Error.prototype.toString);
assert (SyntaxError.prototype.constructor === SyntaxError);
assert (SyntaxError.prototype.name === "SyntaxError");
assert (SyntaxError.prototype.message === "");
assert (SyntaxError.prototype.toString() === "SyntaxError");

/* URIError */
e = new URIError ();
assert (e.name === "URIError");
assert (e.message === "");
assert (e.toString() === "URIError");

e = new URIError("some message");
assert (e.name === "URIError");
assert (e.message === "some message");
assert (e.toString() === "URIError: some message");

e.name = "";
assert (e.toString() === "some message");
e.message = "";
assert (e.toString() === "");

assert (URIError.prototype.toString === Error.prototype.toString);
assert (URIError.prototype.constructor === URIError);
assert (URIError.prototype.name === "URIError");
assert (URIError.prototype.message === "");
assert (URIError.prototype.toString() === "URIError");
