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

var str1 = 'bar\nexample foo example';
var str2 = 'bare\nxample foo example';
var regex_with_dotAll_flag = new RegExp ('bar.example','s');
var regex_without_dotAll_flag = new RegExp ('bar.example');

// testing regexp.prototype.dotAll
assert (regex_with_dotAll_flag.dotAll == true);
assert (regex_without_dotAll_flag.dotAll == false);

// basic dotAll flag test
assert (str1.replace (regex_with_dotAll_flag,'') == " foo example");
assert (str1.replace (regex_without_dotAll_flag,'') == str1);
assert (str2.replace (regex_with_dotAll_flag, "") == str2);

// testing dotAll with other flag
for (let re of [/^.$/su, /^.$/sum]) {
  assert (re.test("a"));
  assert (re.test("3"));
  assert (re.test("π"));
  assert (re.test("\u2027"));
  assert (re.test("\u0085"));
  assert (re.test("\v"));
  assert (re.test("\f"));
  assert (re.test("\u180E"));
  assert (re.test("\u{10300}"));
  assert (re.test("\n"));
  assert (re.test("\r"));
  assert (re.test("\u2028"));
  assert (re.test("\u2029"));
  assert (re.test("\uD800"));
  assert (re.test("\uDFFF"));
}
