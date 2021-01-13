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

// Keeping test as it is due to the fact this was triggered via a gc related issue

var a= ["", "\0", "\t", "\n", "\v", "\f", "\r", " ", "\u00a0", "\u2028", "\u2029", "\ufeff"]
Array.prototype[4] = 10;

function Test()
{
  a.sort(function() {
    var A = function() { };
    A.prototype.x = 42;
    var o = new Proxy({
        "3": {
          writable:false,
          value:20
        }
      }, /*handler*/ {
        getPrototypeOf: function (val, size, ch) {
          var result = new String(val);
          if (ch == null) {
            ch = " ";
          }
          while (result.length < size) {
            result = ch + result;
          }
          return result;
        }
      }
    );

    o.x = 43;
    var result = "";
    for (var p in o) {
        result += o[p];
    }
    return a | 0;
  });

  throw new EvalError("error");
}

try {
  Test();
  assert(false);
} catch (ex) {
  assert (ex instanceof EvalError);
}
