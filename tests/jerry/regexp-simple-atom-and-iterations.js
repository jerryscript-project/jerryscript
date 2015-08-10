// Copyright 2015 Samsung Electronics Co., Ltd.
// Copyright 2015 University of Szeged.
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

var r;

r = new RegExp ("a");
assert (r.exec ("a") == "a");
assert (r.exec ("b") == undefined);

r = new RegExp ("abc");
assert (r.exec ("abc") == "abc");

r = new RegExp ("a*");
assert (r.exec ("aaa") == "aaa");
assert (r.exec ("b") == "");

r = new RegExp ("a+");
assert (r.exec ("aaa") == "aaa");
assert (r.exec ("b") == undefined);

r = new RegExp ("ab*");
assert (r.exec ("a") == "a");
assert (r.exec ("ab") == "ab");
assert (r.exec ("abbbb") == "abbbb");
assert (r.exec ("bbb") == undefined);

r = new RegExp ("a?");
assert (r.exec ("a") == "a");
assert (r.exec ("b") == "");

r = new RegExp ("a{4}");
assert (r.exec ("aaa") == undefined);
assert (r.exec ("aaaaa") == "aaaa");
assert (r.exec ("aaaa") == "aaaa");

r = new RegExp ("a{2,6}");
assert (r.exec ("a") == undefined);
assert (r.exec ("aa") == "aa");
assert (r.exec ("aaaaaa") == "aaaaaa");
assert (r.exec ("aaaaaaa") == "aaaaaa");

r = new RegExp (".*");
assert (r.exec ("abcdefghijkl") == "abcdefghijkl");

r = /\n/;
assert (r.exec ("\n") == "\n");

assert (/[\12]+/.exec ("1\n\n\n\n\n2") == "\n\n\n\n\n");
assert (/[\1284]+/.exec ("1\n\n8\n4\n\n2") == "\n\n8\n4\n\n");
assert (/[\89]12/.exec ("1\9128123") == "912");
assert (/[\11]/.exec ("1\n\n\t\n\n2") == "\t");
assert (/[\142][\143][\144]/.exec ("abcde") == "bcd");

assert (/\12+/.exec ("1\n\n\n\n\n2") == "\n\n\n\n\n");
assert (/\11/.exec ("1\n\n\t\n\n2") == "\t");
assert (/\142\143\144/.exec ("abcde") == "bcd");
assert (/\942\143\144/.exec ("a942cde") == "942cd");
assert (/\14234/.exec ("b34") == "b34");

assert (/(\d+)\2([abc]+)\1\2/.exec("123abc123abc") == "123abc123abc,123,abc");
assert (/([abc]+)\40([d-f]+)\12\1/.exec("abc def\nabc") == "abc def\nabc,abc,def");

var expected = "8765432911,8,7,6,5,4,3,2,9,1";
assert (/(\d)(\d)(\d)(\d)(\d)(\d)(\d)(\d)\9(\d)\9/.exec("8765432911") == expected);

r = /\c/;
assert (r.exec ("\\c") == "\\c");

r = /[\c]/;
assert (r.exec ("c") == "c");

r = /[\c1]/;
assert (r.exec ("\u0011") == "\u0011");

r = /\c3/;
assert (r.exec ("\\c3") == "\\c3");

r = /\cIasd/;
assert (r.exec ("\tasd") == "\tasd");
