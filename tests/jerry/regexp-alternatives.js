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

r = new RegExp ("a|b");
assert (r.exec("a") == "a");

r = new RegExp ("a|b");
assert (r.exec("b") == "b");

r = new RegExp ("a|b|c");
assert (r.exec("b") == "b");

r = new RegExp ("a|b|c");
assert (r.exec("c") == "c");

r = new RegExp ("a|b|c|d");
assert (r.exec("") == undefined);

r = new RegExp ("a|b|c|d");
assert (r.exec("a") == "a");

r = new RegExp ("a|b|c|d");
assert (r.exec("b") == "b");

r = new RegExp ("a|b|c|d");
assert (r.exec("c") == "c");

r = new RegExp ("a|b|c|d");
assert (r.exec("d") == "d");

r = new RegExp ("a|bb|c|d");
assert (r.exec("e") == undefined);

r = new RegExp ("a|bb|c|d");
assert (r.exec("bb") == "bb");

r = new RegExp ("a|bb|c|d");
assert (r.exec("bba") == "bb");

r = new RegExp ("a|bb|c|d");
assert (r.exec("bbbb") == "bb");

r = new RegExp ("a|bb|c|d");
assert (r.exec("a") == "a");

r = new RegExp ("a|bb|c|d");
assert (r.exec("b") == undefined);
