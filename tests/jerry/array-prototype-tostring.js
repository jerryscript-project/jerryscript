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

// Our own join method if the internal join is not implemented.
function join(sep)
{
  sep = sep ? sep : ",";
  var result = "";

  for (var i = 0; i < this.length; ++i) {
    result += this[i];
    if (i + 1 < this.length) {
      result += sep;
    }
  }

  return result;
}

// Force fallback to object.prototype.toString()
Array.prototype.join = 1;

assert ([1].toString() === "[object Array]");

Array.prototype.join = join;

assert ([1, 2].toString() === "1,2");

var test = [1,2,3];
test.join = function() { throw ReferenceError ("foo"); };

try {
  test.toString();

  assert (false);
} catch (e) {
  assert (e.message === "foo");
  assert (e instanceof ReferenceError);
}


// Test if the join returns a ReferenceError
var arr = [1,2]
Object.defineProperty(arr, 'join', { 'get' : function () {throw new ReferenceError ("foo"); } });
try {
  arr.toString();

  assert (false);
} catch (e) {
  assert (e.message === "foo");
  assert (e instanceof ReferenceError);
}
