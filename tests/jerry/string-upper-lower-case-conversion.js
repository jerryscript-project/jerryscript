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

// Conversion

assert ("0123456789abcdefghijklmnopqrstuvwxzyABCDEFGHIJKLMNOPQRSTUVWXYZ".toLowerCase()
        == "0123456789abcdefghijklmnopqrstuvwxzyabcdefghijklmnopqrstuvwxyz");
assert ("0123456789abcdefghijklmnopqrstuvwxzyABCDEFGHIJKLMNOPQRSTUVWXYZ".toUpperCase()
        == "0123456789ABCDEFGHIJKLMNOPQRSTUVWXZYABCDEFGHIJKLMNOPQRSTUVWXYZ");

// Although codepoint 0x10400 and 0x10428 are an upper-lowercase pair,
// we must not do their conversion in JavaScript. We must also ignore
// stray surrogates.

assert ("\ud801\ud801\udc00\udc00".toLowerCase() == "\ud801\ud801\udc00\udc00");
assert ("\ud801\ud801\udc28\udc28".toUpperCase() == "\ud801\ud801\udc28\udc28");

// Conversion of non-string objects.

assert (String.prototype.toUpperCase.call(true) == "TRUE");
assert (String.prototype.toLowerCase.call(-23) == "-23");

var object = { toString : function() { return "<sTr>"; } };
assert (String.prototype.toUpperCase.call(object) == "<STR>");
assert (String.prototype.toLowerCase.call(object) == "<str>");

try
{
  String.prototype.toUpperCase.call(null);
  assert(false);
}
catch (e)
{
  assert (e instanceof TypeError);
}
