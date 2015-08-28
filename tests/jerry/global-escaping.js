// Copyright 2015 University of Szeged
// Copyright 2015 Samsung Electronics Co., Ltd.
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

// Escaping

assert (escape ("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f") ===
        "%00%01%02%03%04%05%06%07%08%09%0A%0B%0C%0D%0E%0F");
assert (escape ("\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f") ===
        "%10%11%12%13%14%15%16%17%18%19%1A%1B%1C%1D%1E%1F");
assert (escape (" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMN") ===
        "%20%21%22%23%24%25%26%27%28%29*+%2C-./0123456789%3A%3B%3C%3D%3E%3F@ABCDEFGHIJKLMN");
assert (escape ("OPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}\x7F") ===
        "OPQRSTUVWXYZ%5B%5C%5D%5E_%60abcdefghijklmnopqrstuvwxyz%7B%7C%7D%7F");

assert (escape("\x80\x95\xaf\xfe\xff") === "%80%95%AF%FE%FF");
assert (escape("\u0100\ud800\udc00") === "%u0100%uD800%uDC00");

assert (escape({}) === "%5Bobject%20Object%5D");
assert (escape(true) === "true");

// Unescaping

assert (unescape ("%00%01%02%03%04%05%06%07%08%09%0A%0B%0C%0D%0E%0F") ===
        "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f");
assert (unescape("%10%11%12%13%14%15%16%17%18%19%1A%1B%1C%1D%1E%1F") ===
        "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f");
assert (unescape("%20%21%22%23%24%25%26%27%28%29*+%2C-./0123456789%3A%3B%3C%3D%3E%3F@ABCDEFGHIJKLMN") ===
        " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMN");
assert (unescape("OPQRSTUVWXYZ%5B%5C%5D%5E_%60abcdefghijklmnopqrstuvwxyz%7B%7C%7D%7F") ===
        "OPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}\x7F");
assert (unescape("%80%95%AF%FE%FF") === "\x80\x95\xaf\xfe\xff");
assert (unescape("%ud800") === "\ud800");
assert (unescape("\ud800") === "\ud800");
assert (unescape("\ud800\udc00") === "\ud800\udc00");
assert (unescape("%ud800%udc00") === "\ud800\udc00");
assert (unescape("\ud800%udc00") === "\ud800\udc00");
assert (unescape("%ud800\udc00") === "\ud800\udc00");

assert (unescape({}) === "[object Object]");
assert (unescape(true) === "true")
assert (unescape() === "undefined");
assert (unescape(1985) === "1985");
assert (unescape("%5#%uu") === "%5#%uu");

// Inversion

var str = "\u0001\u0000\uFFFF";
assert (unescape(escape(str)) === str);
