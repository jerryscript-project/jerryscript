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

var result = /\0/.exec("\u0000");
assert (result !== null);
assert (result[0] === "\u0000");

result = /\0/u.exec("\u0000");
assert (result !== null);
assert (result[0] === "\u0000");

result = /\000/.exec("\u0000");
assert (result !== null);
assert (result[0] === "\u0000");

try {
  new RegExp("\\000", 'u').exec("\u0000");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

result = /\0000/.exec("\u0000\u0030");
assert (result !== null);
assert (result[0] === "\u0000\u0030");

result = /\377/.exec("\u00ff");
assert (result !== null);
assert (result[0] === "\u00ff");

try {
  new RegExp("\\377", 'u').exec("\u00ff");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

result = /\3777/.exec("\u00ff\u0037");
assert (result !== null);
assert (result[0] === "\u00ff\u0037");

try {
  new RegExp("\\3777", 'u').exec("\u00ff\u0037");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

result = /\400/.exec("\u0020\u0030");
assert (result !== null);
assert (result[0] === "\u0020\u0030");

try {
  new RegExp("\\400", 'u').exec("\u0020\u0030");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

result = /(\1)/.exec("\u0001");
assert (result !== null);
assert (result[0].length === 0);

result = /(\1)/u.exec("\u0001");
assert (result !== null);
assert (result[0].length === 0);

result = /(\2)/.exec("\u0002");
assert (result !== null);
assert (result[0] === '\u0002');

try {
  new RegExp("(\\2)", 'u').exec("\u0002");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

result = /\8/.exec("\u0038");
assert (result !== null);
assert (result[0] === '8');

result = /\99/.exec("\u0039\u0039");
assert (result !== null);
assert (result[0] === "99");

// CharClassEscape
assert (/\d+/.exec("123")[0] === "123");
assert (/\D+/.exec("abc")[0] === "abc");
assert (/\s+/.exec("   ")[0] === "   ");
assert (/\S+/.exec("abc")[0] === "abc");
assert (/\w+/.exec("abc")[0] === "abc");
assert (/\W+/.exec("|||")[0] === "|||");
assert (/\d+/u.exec("123")[0] === "123");
assert (/\D+/u.exec("abc")[0] === "abc");
assert (/\s+/u.exec("   ")[0] === "   ");
assert (/\S+/u.exec("abc")[0] === "abc");
assert (/\w+/u.exec("abc")[0] === "abc");
assert (/\W+/u.exec("|||")[0] === "|||");

assert (/\d+/u.exec("\u{10CAF}") === null);
assert (/\D+/u.exec("\u{10CAF}")[0] === "\u{10CAF}");
assert (/\s+/u.exec("\u{10CAF}") === null);
assert (/\S+/u.exec("\u{10CAF}")[0] === "\u{10CAF}");
assert (/\w+/u.exec("\u{10CAF}") === null);
assert (/\W+/u.exec("\u{10CAF}")[0] === "\u{10CAF}");

result = /\xz/.exec("xz");
assert (result !== null);
assert (result[0] === "xz");

try {
  new RegExp("\\xz", "u").exec("xz");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

result = /\c/.exec("\\c");
assert (result !== null);
assert (result[0] === "\\c");

try {
  new RegExp("\\c", 'u').exec("\\c")
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

result = /\c1/.exec("\\c1");
assert (result !== null);
assert (result[0] === "\\c1");

try {
  new RegExp("\\c1", 'u').exec("\\c1");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

try {
  new RegExp("^+");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

try {
  new RegExp("$+");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

try {
  new RegExp("\\b+");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

try {
  new RegExp("\\B+");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

assert (/[\b]/.exec("\u0008")[0] === "\u0008");
assert (/[\b]/u.exec("\u0008")[0] === "\u0008");
assert (/[\B]/.exec("\u0042")[0] === "\u0042");

try {
  new RegExp ("[\\B]", 'u').exec("\u0042");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

assert (/[\c1]/.exec("\u0011")[0] === "\u0011");
assert (/[\c_]/.exec("\u001f")[0] === "\u001f");
assert (/[\c]/.exec("\\")[0] === "\\");
assert (/[\c]/.exec("c")[0] === "c");

try {
  new RegExp("[\\c1]", 'u');
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

try {
  new RegExp("[\\c]", 'u');
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

try {
  new RegExp("[\\c_]", 'u');
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

assert (/{{1,2}/.exec("{{")[0] === "{{");

try {
  new RegExp("{{1,2}", 'u').exec("{{");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

assert (/a{1,2/.exec("a{1,2")[0] === "a{1,2");

try {
  new RegExp("a{1,2", 'u').exec("a{1,2");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

assert (/\u017f/i.exec("s") === null);
assert (/\u017f/ui.exec("s")[0] === "s");

assert (/𐲯/.exec("𐲯")[0] === "𐲯");
assert (/𐲯/u.exec("𐲯")[0] === "𐲯");
assert (/𐲯*?/.exec("𐲯")[0] === "\ud803");
assert (/𐲯*?/u.exec("𐲯")[0] === "");
assert (/𐲯+/.exec("𐲯𐲯𐲯")[0] === "𐲯");
assert (/𐲯+/u.exec("𐲯𐲯𐲯")[0] === "𐲯𐲯𐲯");

assert (/\ud803\udc96*?/.exec("𐲖")[0] === '\ud803');
assert (/\ud803\udc96*?/u.exec("𐲖")[0] === '');
assert (/\ud803\udc96+/.exec("𐲖𐲖𐲖")[0] === '𐲖');
assert (/\ud803\udc96+/u.exec("𐲖𐲖𐲖")[0] === '𐲖𐲖𐲖');

assert (/.*𐲗𐲘/u.exec("𐲓𐲔𐲕𐲖𐲗𐲘")[0] === '𐲓𐲔𐲕𐲖𐲗𐲘');

assert (/[\u{10000}]/.exec("\u{10000}") === null);
assert (/[\u{10000}]/.exec("{")[0] === "{");
assert (/[^\u{10000}]/.exec("\u{10000}")[0] === "\ud800");
assert (/[^\u{10000}]/.exec("{") === null);

assert (/[\uffff]/.exec("\uffff")[0] === "\uffff");
assert (/[^\uffff]/.exec("\uffff") === null);

assert (/[\u{10000}]/u.exec("\u{10000}")[0] === "\u{10000}");
assert (/[\u{10000}]/u.exec("{") === null);
assert (/[^\u{10000}]/u.exec("\u{10000}") === null);
assert (/[^\u{10000}]/u.exec("{")[0] === "{");

assert (/[\uffff]/u.exec("\uffff")[0] === "\uffff");
assert (/[^\uffff]/u.exec("\uffff") === null);

assert (/a{4294967296,4294967297}/.exec("aaaa") === null);
assert (/a{4294967294,4294967295}/.exec("aaaa") === null);
assert (/a{0000000000000000001,0000000000000000002}/u.exec("aaaa")[0] === 'aa');
assert (/(\4294967297)/.exec("\4294967297")[0] === "\4294967297");
assert (/(\1)/u.exec("aaaa")[0] === "");

try {
  new RegExp("a{4294967295,4294967294}", '');
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

assert (/[\d-\s]/.exec("-")[0] === "-");
assert (/[0-\s]/.exec("-")[0] === "-");
assert (/[\d-0]/.exec("-")[0] === "-");

try {
  new RegExp("[\\d-\\s]", 'u').exec("-");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

try {
  new RegExp("[0-\\s]", 'u').exec("-");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

try {
  new RegExp("[\\d-0]", 'u').exec("-");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

assert (/[-]/.exec("-")[0] === "-");
assert (/[-]/u.exec("-")[0] === "-");
assert (/[--]/.exec("-")[0] === "-");
assert (/[--]/u.exec("-")[0] === "-");

assert (/}/.exec("}")[0] === "}");
assert (/\}/u.exec("}")[0] === "}");

try {
  new RegExp("}", 'u').exec("}");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

assert (/]/.exec("]")[0] === "]");
assert (/\]/u.exec("]")[0] === "]");

try {
  new RegExp("]", 'u').exec("]");
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

assert (/(?=)*/.exec("")[0] === "");
assert (/(?=)+/.exec("")[0] === "");
assert (/(?=){1,2}/.exec("")[0] === "");

try {
  new RegExp("(?=)*", 'u');
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

try {
  new RegExp("(?=)+", 'u');
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

try {
  new RegExp("(?=){1,2}", 'u');
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

try {
  new RegExp("(?=){2,1}", '');
  assert (false);
} catch (e) {
  assert (e instanceof SyntaxError);
}

assert(/\w/iu.test("ſ"));
assert(/\w/iu.test("\u212a"));
assert(/k/iu.test("\u212a"));
assert(/\u{10c90}/iu.test("\u{10cd0}"));

assert(/\b/iu.test("ſ"));
assert(/\b/iu.test("\u212a"));
assert(/.\B/iu.test("aſ"));
assert(/.\B/iu.test("a\u212a"));
