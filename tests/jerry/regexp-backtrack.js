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

assert (JSON.stringify (/(?:(a)*){3,}/.exec("aaaab")) === '["aaaa",null]');
assert (JSON.stringify (/((a)*){3,}/.exec("aaaab")) === '["aaaa","",null]');
assert (JSON.stringify (/((a)+){3,}/.exec("aaaab")) === '["aaaa","a","a"]');
assert (JSON.stringify (/((.)*){3,}/.exec("abcd")) === '["abcd","",null]');
assert (JSON.stringify (/((.)+){3,}/.exec("abcd")) === '["abcd","d","d"]');

assert (JSON.stringify (/((.){1,2}){1,2}/.exec("abc")) === '["abc","c","c"]');
assert (JSON.stringify (/(?:(a)*?)asd/.exec("aaasd")) === '["aaasd","a"]');
assert (JSON.stringify (/(?:(a)*)asd/.exec("aaasd")) === '["aaasd","a"]');

assert (JSON.stringify (/(.)*((a)*|(b)*)/.exec("ab")) === '["ab","b","",null,null]');
assert (JSON.stringify (/(.)*((x)|(y))+/.exec("xy")) === '["xy","x","y",null,"y"]');
assert (JSON.stringify (/(.)*((y)|(x))+/.exec("xy")) === '["xy","x","y","y",null]');

assert (JSON.stringify (/((?:a)*)/.exec("aaaad")) === '["aaaa","aaaa"]');
assert (JSON.stringify (/((y)+|x)+/.exec("x")) === '["x","x",null]');
assert (JSON.stringify (/((?:y)*|x)+/.exec("x")) === '["x","x"]');
assert (JSON.stringify (/((y)*|x)+/.exec("x")) === '["x","x",null]');
assert (JSON.stringify (/((y)*|x)*/.exec("x")) === '["x","x",null]');
assert (JSON.stringify (/(?:(y)*|x)*/.exec("x")) === '["x",null]');
assert (JSON.stringify (/(?:(y)*|(x))*/.exec("x")) === '["x",null,"x"]');

assert (JSON.stringify (/((?:a)*)asd/.exec("aaasd")) === '["aaasd","aa"]');
assert (JSON.stringify (/((?:a)+)asd/.exec("aaasd")) === '["aaasd","aa"]');
assert (JSON.stringify (/((?:a)*?)asd/.exec("aaasd")) === '["aaasd","aa"]');
assert (JSON.stringify (/((?:a)+?)asd/.exec("aaasd")) === '["aaasd","aa"]');

assert (JSON.stringify (/((y)|(z)|(a))*/.exec("yazx")) === '["yaz","z",null,"z",null]');
assert (JSON.stringify (/((y)|(z)|(.))*/.exec("yaz")) === '["yaz","z",null,"z",null]');
assert (JSON.stringify (/((y)*|(z)*|(a)*)*/.exec("yazx")) === '["yaz","z",null,"z",null]')
assert (JSON.stringify (/((y)|(z)|(a))*/.exec("yazx")) === '["yaz","z",null,"z",null]')
assert (JSON.stringify (/(?:(y)|(z)|(a))*/.exec("yazx")) === '["yaz",null,"z",null]')
assert (JSON.stringify (/((y)|(z)|(a))+?/.exec("yazx")) === '["y","y","y",null,null]')
assert (JSON.stringify (/(?:(y)|(z)|(a))+?/.exec("yazx")) === '["y","y",null,null]')

assert (JSON.stringify (/(?:(x|y)*|z)*/.exec("yz")) === '["yz",null]');
assert (JSON.stringify (/((x|y)*|z)*/.exec("yz")) == '["yz","z",null]');
assert (JSON.stringify (/(((x|y)*|(v|w)*|z)*)asd/.exec("xyzwvxzasd")) === '["xyzwvxzasd","xyzwvxz","z",null,null]');

assert (JSON.stringify (/((a)*){1,3}b/.exec("ab")) === '["ab","a","a"]')
assert (JSON.stringify (/((a)*){2,3}b/.exec("ab")) === '["ab","",null]')
assert (JSON.stringify (/((a)*){3,3}b/.exec("ab")) === '["ab","",null]')

assert (JSON.stringify (/((a)*){3,}b/.exec("aaaab")) === '["aaaab","",null]');
assert (JSON.stringify (/((a)*)*b/.exec("aaaab")) === '["aaaab","aaaa","a"]');

assert (JSON.stringify (/((bb?)*)*a/.exec("bbba")) === '["bbba","bbb","b"]');
assert (JSON.stringify (/((b)*)*a/.exec("bbba")) === '["bbba","bbb","b"]');

assert (JSON.stringify (/(aa|a)a/.exec("aa")) === '["aa","a"]');
assert (JSON.stringify (/(aa|a)?a/.exec("aa")) === '["aa","a"]');
assert (JSON.stringify (/(aa|a)+?a/.exec("aa")) === '["aa","a"]');
assert (JSON.stringify (/(?:aa|a)a/.exec("aa")) === '["aa"]');
assert (JSON.stringify (/(?:aa|a)?a/.exec("aa")) === '["aa"]');
assert (JSON.stringify (/(?:aa|a)+?a/.exec("aa")) === '["aa"]');

assert (JSON.stringify (/(aa|a)a/.exec("a")) === 'null');
assert (JSON.stringify (/(aa|a)?a/.exec("a")) === '["a",null]');
assert (JSON.stringify (/(aa|a)+?a/.exec("a")) === 'null');
assert (JSON.stringify (/(?:aa|a)a/.exec("a")) === 'null');
assert (JSON.stringify (/(?:aa|a)?a/.exec("a")) === '["a"]');
assert (JSON.stringify (/(?:aa|a)+?a/.exec("a")) === 'null');

assert (JSON.stringify (/a+/.exec("aaasd")) === '["aaa"]');
assert (JSON.stringify (/a+?/.exec("aaasd")) === '["a"]');

assert (JSON.stringify (/a+sd/.exec("aaasd")) === '["aaasd"]');
assert (JSON.stringify (/a+?sd/.exec("aaasd")) === '["aaasd"]');

assert (JSON.stringify (/a{2}sd/.exec("aaasd")) === '["aasd"]');
assert (JSON.stringify (/a{3}sd/.exec("aaasd")) === '["aaasd"]');

assert (JSON.stringify (/(?=a)/.exec("a")) === '[""]');
assert (JSON.stringify (/(?=a)+/.exec("a")) === '[""]');
assert (JSON.stringify (/(?=a)*/.exec("a")) === '[""]');
assert (JSON.stringify (/(?=(a))?/.exec("a")) === '["",null]');
assert (JSON.stringify (/(?=(a))+?/.exec("a")) === '["","a"]');
assert (JSON.stringify (/(?=(a))*?/.exec("a")) === '["",null]');

assert (JSON.stringify (/(?!a)/.exec("a")) === '[""]');
assert (JSON.stringify (/(?!a)+/.exec("a")) === '[""]');
assert (JSON.stringify (/(?!a)*/.exec("a")) === '[""]');
assert (JSON.stringify (/(?!(a))?/.exec("a")) === '["",null]');
assert (JSON.stringify (/(?!(a))+?/.exec("a")) === '["",null]');
assert (JSON.stringify (/(?!(a))*?/.exec("a")) === '["",null]');

assert (JSON.stringify (/al(?=(ma))*ma/.exec("alma")) === '["alma",null]');
assert (JSON.stringify (/al(?!(ma))*ma/.exec("alma")) === '["alma",null]');
assert (JSON.stringify (/al(?=(ma))+ma/.exec("alma")) === '["alma","ma"]');
assert (JSON.stringify (/al(?!(ma))+ma/.exec("alma")) === 'null');

assert (JSON.stringify (/(?=())x|/.exec("asd")) === '["",null]');
assert (JSON.stringify (/(?!())x|/.exec("asd")) === '["",null]');

assert (JSON.stringify (/(().*)+.$/.exec("abcdefg")) === '["abcdefg","abcdef",""]');
assert (JSON.stringify (/(().*)+?.$/.exec("abcdefg")) === '["abcdefg","abcdef",""]');
assert (JSON.stringify (/(?:().*)+.$/.exec("abcdefg")) === '["abcdefg",""]');
assert (JSON.stringify (/(?:().*)+?.$/.exec("abcdefg")) === '["abcdefg",""]');

assert (JSON.stringify(/((?=())|.)+^/.exec("a")) === '["","",""]');
assert (JSON.stringify(/(?:(|\b\w+?){2})+$/.exec("aaaa")) === '["aaaa","aaaa"]');
