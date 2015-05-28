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

// Simple test cases
r = new RegExp ("(?:)");
assert (r.exec ("a") == "");

r = new RegExp ("(?:a)");
assert (r.exec ("a") == "a");

r = new RegExp ("(?:(?:a)b)c");
assert (r.exec ("abc") == "abc");

r = new RegExp ("(?:a)*");
assert (r.exec ("b") == "");
assert (r.exec ("aaaa") == "aaaa");

r = new RegExp ("(?:a)+");
assert (r.exec ("aaaa") == "aaaa");

r = new RegExp ("(?:a){4}");
assert (r.exec ("aaaa") == "aaaa");

r = new RegExp ("(?:a){1,2}");
assert (r.exec ("a") == "a");
assert (r.exec ("aa") == "aa");
assert (r.exec ("aaaa") == "aa");

r = new RegExp ("(?:a)?");
assert (r.exec ("a") == "a");
assert (r.exec ("b") == "");

// Test greedy iterations
// FIXME: Add testcases

// Test non-greedy iterations
// FIXME: Add testcases