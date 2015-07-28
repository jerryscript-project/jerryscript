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

r = new RegExp ("[abc]*").exec("aaabbcccabcacbacabacbacab");
assert (r == "aaabbcccabcacbacabacbacab");

r = new RegExp ("[abc]*").exec("aaabbcccabdcacb");
assert (r == "aaabbcccab");

r = new RegExp ("[abc]*").exec("defghjklmnopqrstuvwxyz");
assert (r == "");

r = new RegExp ("[a-z]*").exec("abcdefghjklmnopqrstuvwxyz");
assert (r == "abcdefghjklmnopqrstuvwxyz");

r = new RegExp ("[A-Z]*").exec("abcdefghjklmnopqrstuvwxyz");
assert (r == "");

r = new RegExp ("[^a-z]*").exec("abcdefghjklmnopqrstuvwxyz");
assert (r == "");

r = new RegExp ("[^A-Z]*").exec("abcdefghjklmnopqrstuvwxyz");
assert (r == "abcdefghjklmnopqrstuvwxyz");

r = new RegExp ("\\d*").exec("abcdefghjklmnopqrstuvwxyz");
assert (r == "");

r = new RegExp ("\\D*").exec("abcdefghjklmnopqrstuvwxyz");
assert (r == "abcdefghjklmnopqrstuvwxyz");

r = new RegExp ("\\w*").exec("abcdefghjklmnopqrstuvwxyz");
assert (r == "abcdefghjklmnopqrstuvwxyz");

r = new RegExp ("\\W*").exec("abcdefghjklmnopqrstuvwxyz");
assert (r == "");

r = new RegExp ("\\s*").exec("abcdefghjklmnopqrstuvwxyz");
assert (r == "");

r = new RegExp ("\\S*").exec("abcdefghjklmnopqrstuvwxyz");
assert (r == "abcdefghjklmnopqrstuvwxyz");

r = new RegExp ("[\\d]*").exec("abcdefghjklmnopqrstuvwxyz");
assert (r == "");

r = new RegExp ("[\\D]*").exec("abcdefghjklmnopqrstuvwxyz");
assert (r == "abcdefghjklmnopqrstuvwxyz");

r = new RegExp ("[\\w]*").exec("abcdefghjklmnopqrstuvwxyz");
assert (r == "abcdefghjklmnopqrstuvwxyz");

r = new RegExp ("[\\W]*").exec("abcdefghjklmnopqrstuvwxyz");
assert (r == "");

r = new RegExp ("[\\s]*").exec("abcdefghjklmnopqrstuvwxyz");
assert (r == "");

r = new RegExp ("[\\S]*").exec("abcdefghjklmnopqrstuvwxyz");
assert (r == "abcdefghjklmnopqrstuvwxyz");

r = new RegExp ("[^\\d]*").exec("abcdefghjklmnopqrstuvwxyz");
assert (r == "abcdefghjklmnopqrstuvwxyz");

r = new RegExp ("[^\\D]*").exec("abcdefghjklmnopqrstuvwxyz");
assert (r == "");

r = new RegExp ("[^\\w]*").exec("abcdefghjklmnopqrstuvwxyz");
assert (r == "");

r = new RegExp ("[^\\W]*").exec("abcdefghjklmnopqrstuvwxyz");
assert (r == "abcdefghjklmnopqrstuvwxyz");

r = new RegExp ("[^\\s]*").exec("abcdefghjklmnopqrstuvwxyz");
assert (r == "abcdefghjklmnopqrstuvwxyz");

r = new RegExp ("[^\\S]*").exec("abcdefghjklmnopqrstuvwxyz");
assert (r == "");

r = new RegExp ("\\d*").exec("0123456789");
assert (r == "0123456789");

try
{
  r = new RegExp("[");
  assert (false);
}
catch (e)
{
  assert (e instanceof SyntaxError);
}

try
{
  r = new RegExp("[\\");
  assert (false);
}
catch (e)
{
  assert (e instanceof SyntaxError);
}
