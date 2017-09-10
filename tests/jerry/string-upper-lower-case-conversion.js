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

// LATIN SMALL LIGATURES
// LATIN SMALL LIGATURE FF
assert ("\ufb00".toLowerCase() == "\ufb00");
assert ("\ufb00".toUpperCase() == "\u0046\u0046");
// LATIN SMALL LIGATURE FI
assert ("\ufb01".toLowerCase() == "\ufb01");
assert ("\ufb01".toUpperCase() == "\u0046\u0049");
// LATIN SMALL LIGATURE FL
assert ("\ufb02".toLowerCase() == "\ufb02");
assert ("\ufb02".toUpperCase() == "\u0046\u004c");
// LATIN SMALL LIGATURE FFI
assert ("\ufb03".toLowerCase() == "\ufb03");
assert ("\ufb03".toUpperCase() == "\u0046\u0046\u0049");
// LATIN SMALL LIGATURE FFL
assert ("\ufb04".toLowerCase() == "\ufb04");
assert ("\ufb04".toUpperCase() == "\u0046\u0046\u004c");
// LATIN SMALL LIGATURE LONG S T
assert ("\ufb05".toLowerCase() == "\ufb05");
assert ("\ufb05".toUpperCase() == "\u0053\u0054");
// LATIN SMALL LIGATURE ST
assert ("\ufb06".toLowerCase() == "\ufb06");
assert ("\ufb06".toUpperCase() == "\u0053\u0054");

// LATIN CAPITAL LETTER I WITH DOT ABOVE
assert ("\u0130".toLowerCase() == "\u0069\u0307");
assert ("\u0130".toUpperCase() == "\u0130");

// LATIN SMALL LETTER SHARP S
assert ("\u00df".toLowerCase() == "\u00df");
assert ("\u00df".toUpperCase() == "\u0053\u0053");

// LATIN CAPITAL LETTER I WITH BREVE
assert ("\u012c".toLowerCase() == "\u012d");
assert ("\u012c".toUpperCase() == "\u012c");
// LATIN SMALL LETTER I WITH BREVE
assert ("\u012d".toLowerCase() == "\u012d")
assert ("\u012d".toUpperCase() == "\u012c");

// Check randomly selected characters from conversion tables

// lower-case conversions
assert ("\u01c5\u01c8\u01cb\u212b".toLowerCase() == "\u01c6\u01c9\u01cc\u00e5");
assert ("\u0130".toLowerCase() == "\u0069\u0307");

// upper-case conversions
assert ("\u00b5\u017f".toUpperCase() == "\u039c\u0053");
assert ("\ufb17\u00df\u1fbc".toUpperCase() == "\u0544\u053D\u0053\u0053\u0391\u0399");
assert ("\ufb03\ufb04".toUpperCase() == "\u0046\u0046\u0049\u0046\u0046\u004c");

// character case ranges
assert ("\u0100\u0101\u0139\u03fa\ua7b4".toLowerCase() == "\u0101\u0101\u013a\u03fb\ua7b5");
assert ("\u0101\u0100\u013a\u03fb\ua7b5".toUpperCase() == "\u0100\u0100\u0139\u03fa\ua7b4");

// character pairs
assert ("\u0178\ua7b1\u0287\ua7b3".toLowerCase() == "\u00ff\u0287\u0287\uab53");
assert ("\u00ff\u0287\ua7b1\uab53".toUpperCase() == "\u0178\ua7b1\ua7b1\ua7b3");

// character case ranges
assert ("\u00e0\u00c0\u00c1\u00c2\uff21".toLowerCase() == "\u00e0\u00e0\u00e1\u00e2\uff41");
assert ("\u00e0\u00c0\u00e1\u00e2\uff41".toUpperCase() == "\u00c0\u00c0\u00c1\u00c2\uff21");

// lower-case ranges
assert ("\u1f88\u1f98\u1fa8\u1f8b\u1faf".toLowerCase() == "\u1f80\u1f90\u1fa0\u1f83\u1fa7");

// upper-case special ranges
assert ("\u1f80\u1f81\u1fa7".toUpperCase() == "\u1f08\u0399\u1f09\u0399\u1f6f\u0399");

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
