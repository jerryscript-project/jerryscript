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

assert ('abcd\
efgh' === 'abcdefgh');

assert ('\'' === "'");
assert ("\'" === "'");
assert ('\"' === '"');
assert ("\"" === '"');

assert ((new String ('\\')).length === 1);
assert ((new String ('\b')).length === 1);
assert ((new String ('\f')).length === 1);
assert ((new String ('\n')).length === 1);
assert ((new String ('\r')).length === 1);
assert ((new String ('\t')).length === 1);
assert ((new String ('\v')).length === 1);

/* verifying character codes */
assert ((new String ('\\')).charCodeAt (0) === 92);
assert ((new String ('\b')).charCodeAt (0) === 8);
assert ((new String ('\f')).charCodeAt (0) === 12);
assert ((new String ('\n')).charCodeAt (0) === 10);
assert ((new String ('\r')).charCodeAt (0) === 13);
assert ((new String ('\t')).charCodeAt (0) === 9);
assert ((new String ('\v')).charCodeAt (0) === 11);

/* 'p' is not SingleEscapeCharacter */
assert ('\p' === 'p');

var v\u0061riable = 'valu\u0065';
assert (variable === 'value');
