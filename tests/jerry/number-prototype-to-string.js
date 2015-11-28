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

assert((NaN).toString() === "NaN");
assert((-Infinity).toString() === "-Infinity");
assert((Infinity).toString() === "Infinity");
assert((NaN).toString(6) === "NaN");
assert((-Infinity).toString(7) === "-Infinity");
assert((Infinity).toString(8) === "Infinity");
assert((16).toString(16) === "10");
assert((15).toString(16) === "f");
assert((12.5).toString(4) === "30.2");
assert((0.005).toString(4) === "0.000110132232011013223201101323");
assert((2000).toString(4) === "133100");
assert((2000).toString(3) === "2202002");
assert((2000).toString(16) === "7d0");
assert((0.03125).toString(2) === "0.00001");
assert((0.03125).toString(16) === "0.08");
assert((0.0001).toString(4) === "0.000000122031232023223013010030231")
assert((0).toString(16) === "0");
assert((-16).toString(16) === "-10");
assert((-15).toString(16) === "-f");
assert((-12.5).toString(4) === "-30.2");
assert((-0.005).toString(4) === "-0.000110132232011013223201101323");
assert((-2000).toString(4) === "-133100");
assert((-2000).toString(3) === "-2202002");
assert((-2000).toString(16) === "-7d0");
assert((-0.03125).toString(2) === "-0.00001");
assert((-0.03125).toString(16) === "-0.08");
assert((-0.0001).toString(4) === "-0.000000122031232023223013010030231")
assert((-0).toString(16) === "0");
assert((1e+73).toString(35) === "2nx1mg1l0w4ujlpt449c5qfrkkmtpgpsfsc2prlaqtnjbli2")
assert((-1e+73).toString(35) === "-2nx1mg1l0w4ujlpt449c5qfrkkmtpgpsfsc2prlaqtnjbli2")
assert((1).toString(undefined) === "1")

assert((123400).toString(2) === "11110001000001000");
assert((123400).toString(3) === "20021021101");
assert((123400).toString(4) === "132020020");
assert((123400).toString(5) === "12422100");
assert((123400).toString(6) === "2351144");
assert((123400).toString(7) === "1022524");
assert((123400).toString(8) === "361010");
assert((123400).toString(9) === "207241");
assert((123400).toString(10) === "123400");
assert((123400).toString(11) === "84792");
assert((123400).toString(12) === "5b4b4");
assert((123400).toString(13) === "44224");
assert((123400).toString(14) === "32d84");
assert((123400).toString(15) === "2686a");
assert((123400).toString(16) === "1e208");
assert((123400).toString(17) === "181ge");
assert((123400).toString(18) === "132fa");
assert((123400).toString(19) === "hife");
assert((123400).toString(20) === "f8a0");
assert((123400).toString(21) === "d6h4");
assert((123400).toString(22) === "bcl2");
assert((123400).toString(23) === "a365");
assert((123400).toString(24) === "8m5g");
assert((123400).toString(25) === "7mb0");
assert((123400).toString(26) === "70e4");
assert((123400).toString(27) === "677a");
assert((123400).toString(28) === "5hb4");
assert((123400).toString(29) === "51l5");
assert((123400).toString(30) === "4h3a");
assert((123400).toString(31) === "44ck");
assert((123400).toString(32) === "3og8");
assert((123400).toString(33) === "3ead");
assert((123400).toString(34) === "34pe");
assert((123400).toString(35) === "2upp");
assert((123400).toString(36) === "2n7s");

assert((123400).toString(new Number(16)) === "1e208");

var digit_chars = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                   'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
                   'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
                   'u', 'v', 'w', 'x', 'y', 'z'];

for (radix = 2; radix <= 36; radix++) {
  for (num = 1; num < 100; num++) {
    var value = num;
    var str = "";
    while (value > 0) {
      str = digit_chars[value % radix] + str;
      value = Math.floor(value / radix);
    }

    assert(str === (num).toString(radix));
  }
}

try {
  assert((123).toString(1));
  assert(false)
} catch (e) {
  assert(e instanceof RangeError);
}

try {
  assert((123).toString(37));
  assert(false)
} catch (e) {
  assert(e instanceof RangeError);
}

try {
  assert((123).toString(Infinity));
  assert(false)
} catch (e) {
  assert(e instanceof RangeError);
}
