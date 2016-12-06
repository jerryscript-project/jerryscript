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

// URI encoding

function checkEncodeURIParseError (str)
{
  try {
    encodeURI (str);
    assert (false);
  } catch(e) {
    assert(e instanceof URIError);
  }
}

assert (encodeURI ("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f") ===
        "%00%01%02%03%04%05%06%07%08%09%0A%0B%0C%0D%0E%0F");
assert (encodeURI ("\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f") ===
        "%10%11%12%13%14%15%16%17%18%19%1A%1B%1C%1D%1E%1F");
assert (encodeURI (" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMN") ===
         "%20!%22#$%25&'()*+,-./0123456789:;%3C=%3E?@ABCDEFGHIJKLMN");
assert (encodeURI ("OPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}\x7F") ===
         "OPQRSTUVWXYZ%5B%5C%5D%5E_%60abcdefghijklmnopqrstuvwxyz%7B%7C%7D%7F");

assert (encodeURIComponent ("\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f") ===
        "%00%01%02%03%04%05%06%07%08%09%0A%0B%0C%0D%0E%0F");
assert (encodeURIComponent ("\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f") ===
        "%10%11%12%13%14%15%16%17%18%19%1A%1B%1C%1D%1E%1F");
assert (encodeURIComponent (" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMN") ===
        "%20!%22%23%24%25%26'()*%2B%2C-.%2F0123456789%3A%3B%3C%3D%3E%3F%40ABCDEFGHIJKLMN");
assert (encodeURIComponent ("OPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}\x7F") ===
        "OPQRSTUVWXYZ%5B%5C%5D%5E_%60abcdefghijklmnopqrstuvwxyz%7B%7C%7D%7F");

assert (encodeURI ("\xe9") == "%C3%A9");
assert (encodeURI ("\ud7ff") == "%ED%9F%BF");
assert (encodeURI ("\ue000") == "%EE%80%80");
assert (encodeURI ("\ud800\udc00") == "%F0%90%80%80");
assert (encodeURI (String.fromCharCode(0xd800, 0xdc00)) == "%F0%90%80%80");

checkEncodeURIParseError ("\ud800");
checkEncodeURIParseError ("\udfff");

// URI decoding

function checkDecodeURIParseError (str)
{
  try {
    decodeURI (str);
    assert (false);
  } catch(e) {
    assert(e instanceof URIError);
  }
}

assert (decodeURI ("%00%01%02%03%04%05%06%07%08%09%0A%0B%0C%0D%0E%0F") ===
        "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f");
assert (decodeURI ("%10%11%12%13%14%15%16%17%18%19%1A%1B%1C%1D%1E%1F") ===
        "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f");
assert (decodeURI ("%20%21%22%23%24%25%26%27%28%29%2a%2b%2c%2d%2e%2f") ===
        " !\"%23%24%%26'()*%2b%2c-.%2f");
assert (decodeURI ("%30%31%32%33%34%35%36%37%38%39%3a%3b%3c%3d%3e%3f") ===
        "0123456789%3a%3b<%3d>%3f");
assert (decodeURI ("%40%41%42%43%44%45%46%47%48%49%4a%4b%4c%4d%4e%4f") ===
        "%40ABCDEFGHIJKLMNO");
assert (decodeURI ("%50%51%52%53%54%55%56%57%58%59%5a%5b%5c%5d%5e%5f") ===
        "PQRSTUVWXYZ[\\]^_");
assert (decodeURI ("%60%61%62%63%64%65%66%67%68%69%6a%6b%6c%6d%6e%6f") ===
        "`abcdefghijklmno");
assert (decodeURI ("%70%71%72%73%74%75%76%77%78%79%7a%7b%7c%7d%7e") ===
        "pqrstuvwxyz{|}~");

assert (decodeURIComponent ("%00%01%02%03%04%05%06%07%08%09%0A%0B%0C%0D%0E%0F") ===
        "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f");
assert (decodeURIComponent ("%10%11%12%13%14%15%16%17%18%19%1A%1B%1C%1D%1E%1F") ===
        "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f");
assert (decodeURIComponent ("%20%21%22%23%24%25%26%27%28%29%2a%2b%2c%2d%2e%2f") ===
        " !\"#$%&'()*+,-./");
assert (decodeURIComponent ("%30%31%32%33%34%35%36%37%38%39%3a%3b%3c%3d%3e%3f") ===
        "0123456789:;<=>?");
assert (decodeURIComponent ("%40%41%42%43%44%45%46%47%48%49%4a%4b%4c%4d%4e%4f") ===
        "@ABCDEFGHIJKLMNO");
assert (decodeURIComponent ("%50%51%52%53%54%55%56%57%58%59%5a%5b%5c%5d%5e%5f") ===
        "PQRSTUVWXYZ[\\]^_");
assert (decodeURIComponent ("%60%61%62%63%64%65%66%67%68%69%6a%6b%6c%6d%6e%6f") ===
        "`abcdefghijklmno");
assert (decodeURIComponent ("%70%71%72%73%74%75%76%77%78%79%7a%7b%7c%7d%7e") ===
        "pqrstuvwxyz{|}~");

assert (decodeURI ("%6A%6B%6C%6D%6E%6F") === "jklmno");
assert (decodeURI ("%C3%A9") === "\xe9");
assert (decodeURI ("%e2%b1%a5") === "\u2c65");
/* assert (decodeURI ("%f0%90%90%a8") === "\ud801\udc28"); */

checkDecodeURIParseError ("13%");
checkDecodeURIParseError ("%0g");
checkDecodeURIParseError ("%1G");
checkDecodeURIParseError ("%a");
checkDecodeURIParseError ("%c1%81");
checkDecodeURIParseError ("a%80b");
checkDecodeURIParseError ("%f4%90%80%80");
checkDecodeURIParseError ("%ed%a0%80");
checkDecodeURIParseError ("%ed%b0%80");
checkDecodeURIParseError ("%fa%81%82%83%84");
checkDecodeURIParseError ("%fd%81%82%83%84%85");

// Coerce (automatic toString()) tests

assert (decodeURI ([1, 2, 3]) === "1,2,3");
assert (decodeURI ({ x:1 }) === "[object Object]");
assert (encodeURI (void 0) === "undefined");
assert (encodeURI (216.000e1) === "2160");

// Combining surrogate fragments

assert (decodeURI("\ud800\udc00 \ud800 \udc00") === "\ud800\udc00 \ud800 \udc00");
assert (decodeURI("%f0%90%80%80") === "\ud800\udc00");
assert (decodeURI("\ud800%f0%90%80%80\ud800") === "\ud800\ud800\udc00\ud800");
assert (decodeURI("\udc00%f0%90%80%80\udc00") === "\udc00\ud800\udc00\udc00");

checkDecodeURIParseError ("\ud800%ed%b0%80");
checkDecodeURIParseError ("%ed%a0%80\udc00");
