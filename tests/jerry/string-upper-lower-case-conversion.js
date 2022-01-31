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

// LATIN SMALL LIGATURES
// LATIN SMALL LIGATURE FF
assert ("\ufb00".toLowerCase() == "\ufb00");
assert ("\ufb00".toUpperCase() == "\u0046\u0046");
// LATIN SMALL LIGATURE FI
assert ("\ufb01".toLowerCase() == "\ufb01");
assert ("\ufb01".toUpperCase() == "\u0046\u0049");
// LATIN SMALL LIGATURE FL
assert ("\ufb02".toLowerCase() == "\ufb02");
assert ("\ufb02".toUpperCase() == "\u0046\u004c");
// LATIN SMALL LIGATURE FFI
assert ("\ufb03".toLowerCase() == "\ufb03");
assert ("\ufb03".toUpperCase() == "\u0046\u0046\u0049");
// LATIN SMALL LIGATURE FFL
assert ("\ufb04".toLowerCase() == "\ufb04");
assert ("\ufb04".toUpperCase() == "\u0046\u0046\u004c");
// LATIN SMALL LIGATURE LONG S T
assert ("\ufb05".toLowerCase() == "\ufb05");
assert ("\ufb05".toUpperCase() == "\u0053\u0054");
// LATIN SMALL LIGATURE ST
assert ("\ufb06".toLowerCase() == "\ufb06");
assert ("\ufb06".toUpperCase() == "\u0053\u0054");

// LATIN CAPITAL LETTER I WITH DOT ABOVE
assert ("\u0130".toLowerCase() == "\u0069\u0307");
assert ("\u0130".toUpperCase() == "\u0130");

// LATIN SMALL LETTER SHARP S
assert ("\u00df".toLowerCase() == "\u00df");
assert ("\u00df".toUpperCase() == "\u0053\u0053");

// LATIN CAPITAL LETTER I WITH BREVE
assert ("\u012c".toLowerCase() == "\u012d");
assert ("\u012c".toUpperCase() == "\u012c");
// LATIN SMALL LETTER I WITH BREVE
assert ("\u012d".toLowerCase() == "\u012d")
assert ("\u012d".toUpperCase() == "\u012c");

// Check randomly selected characters from conversion tables

// lower-case conversions
assert ("\u01c5\u01c8\u01cb\u212b".toLowerCase() == "\u01c6\u01c9\u01cc\u00e5");
assert ("\u0130".toLowerCase() == "\u0069\u0307");

// upper-case conversions
assert ("\u00b5\u017f".toUpperCase() == "\u039c\u0053");
assert ("\ufb17\u00df\u1fbc".toUpperCase() == "\u0544\u053D\u0053\u0053\u0391\u0399");
assert ("\ufb03\ufb04".toUpperCase() == "\u0046\u0046\u0049\u0046\u0046\u004c");

// character case ranges
assert ("\u0100\u0101\u0139\u03fa\ua7b4".toLowerCase() == "\u0101\u0101\u013a\u03fb\ua7b5");
assert ("\u0101\u0100\u013a\u03fb\ua7b5".toUpperCase() == "\u0100\u0100\u0139\u03fa\ua7b4");

// character pairs
assert ("\u0178\ua7b1\u0287\ua7b3".toLowerCase() == "\u00ff\u0287\u0287\uab53");
assert ("\u00ff\u0287\ua7b1\uab53".toUpperCase() == "\u0178\ua7b1\ua7b1\ua7b3");

// character case ranges
assert ("\u00e0\u00c0\u00c1\u00c2\uff21".toLowerCase() == "\u00e0\u00e0\u00e1\u00e2\uff41");
assert ("\u00e0\u00c0\u00e1\u00e2\uff41".toUpperCase() == "\u00c0\u00c0\u00c1\u00c2\uff21");

// lower-case ranges
assert ("\u1f88\u1f98\u1fa8\u1f8b\u1faf".toLowerCase() == "\u1f80\u1f90\u1fa0\u1f83\u1fa7");

// upper-case special ranges
assert ("\u1f80\u1f81\u1fa7".toUpperCase() == "\u1f08\u0399\u1f09\u0399\u1f6f\u0399");

assert ("0123456789abcdefghijklmnopqrstuvwxzyABCDEFGHIJKLMNOPQRSTUVWXYZ".toLowerCase()
        == "0123456789abcdefghijklmnopqrstuvwxzyabcdefghijklmnopqrstuvwxyz");
assert ("0123456789abcdefghijklmnopqrstuvwxzyABCDEFGHIJKLMNOPQRSTUVWXYZ".toUpperCase()
        == "0123456789ABCDEFGHIJKLMNOPQRSTUVWXZYABCDEFGHIJKLMNOPQRSTUVWXYZ");

// Conversion of non-string objects.

assert (String.prototype.toUpperCase.call(true) == "TRUE");
assert (String.prototype.toLowerCase.call(-23) == "-23");

var object = { toString : function() { return "<sTr>"; } };
assert (String.prototype.toUpperCase.call(object) == "<STR>");
assert (String.prototype.toLowerCase.call(object) == "<str>");

try
{
  String.prototype.toUpperCase.call(null);
  assert(false);
}
catch (e)
{
  assert (e instanceof TypeError);
}

let start = 0x10000
let end = 0x10FFFF

const lower_expected = [66560, 66561, 66562, 66563, 66564, 66565, 66566, 66567, 66568, 66569, 66570, 66571, 66572,
                        66573, 66574, 66575, 66576, 66577, 66578, 66579, 66580, 66581, 66582, 66583, 66584, 66585,
                        66586, 66587, 66588, 66589, 66590, 66591, 66592, 66593, 66594, 66595, 66596, 66597, 66598,
                        66599, 66736, 66737, 66738, 66739, 66740, 66741, 66742, 66743, 66744, 66745, 66746, 66747,
                        66748, 66749, 66750, 66751, 66752, 66753, 66754, 66755, 66756, 66757, 66758, 66759, 66760,
                        66761, 66762, 66763, 66764, 66765, 66766, 66767, 66768, 66769, 66770, 66771, 68736, 68737,
                        68738, 68739, 68740, 68741, 68742, 68743, 68744, 68745, 68746, 68747, 68748, 68749, 68750,
                        68751, 68752, 68753, 68754, 68755, 68756, 68757, 68758, 68759, 68760, 68761, 68762, 68763,
                        68764, 68765, 68766, 68767, 68768, 68769, 68770, 68771, 68772, 68773, 68774, 68775, 68776,
                        68777, 68778, 68779, 68780, 68781, 68782, 68783, 68784, 68785, 68786, 71840, 71841, 71842,
                        71843, 71844, 71845, 71846, 71847, 71848, 71849, 71850, 71851, 71852, 71853, 71854, 71855,
                        71856, 71857, 71858, 71859, 71860, 71861, 71862, 71863, 71864, 71865, 71866, 71867, 71868,
                        71869, 71870, 71871, 93760, 93761, 93762, 93763, 93764, 93765, 93766, 93767, 93768, 93769,
                        93770, 93771, 93772, 93773, 93774, 93775, 93776, 93777, 93778, 93779, 93780, 93781, 93782,
                        93783, 93784, 93785, 93786, 93787, 93788, 93789, 93790, 93791, 125184, 125185, 125186, 125187,
                        125188, 125189, 125190, 125191, 125192, 125193, 125194, 125195, 125196, 125197, 125198, 125199,
                        125200, 125201, 125202, 125203, 125204, 125205, 125206, 125207, 125208, 125209, 125210, 125211,
                        125212, 125213, 125214, 125215, 125216, 125217];

const upper_expected = [66600, 66601, 66602, 66603, 66604, 66605, 66606, 66607, 66608, 66609, 66610, 66611, 66612,
                        66613, 66614, 66615, 66616, 66617, 66618, 66619, 66620, 66621, 66622, 66623, 66624, 66625,
                        66626, 66627, 66628, 66629, 66630, 66631, 66632, 66633, 66634, 66635, 66636, 66637, 66638,
                        66639, 66776, 66777, 66778, 66779, 66780, 66781, 66782, 66783, 66784, 66785, 66786, 66787,
                        66788, 66789, 66790, 66791, 66792, 66793, 66794, 66795, 66796, 66797, 66798, 66799, 66800,
                        66801, 66802, 66803, 66804, 66805, 66806, 66807, 66808, 66809, 66810, 66811, 68800, 68801,
                        68802, 68803, 68804, 68805, 68806, 68807, 68808, 68809, 68810, 68811, 68812, 68813, 68814,
                        68815, 68816, 68817, 68818, 68819, 68820, 68821, 68822, 68823, 68824, 68825, 68826, 68827,
                        68828, 68829, 68830, 68831, 68832, 68833, 68834, 68835, 68836, 68837, 68838, 68839, 68840,
                        68841, 68842, 68843, 68844, 68845, 68846, 68847, 68848, 68849, 68850, 71872, 71873, 71874,
                        71875, 71876, 71877, 71878, 71879, 71880, 71881, 71882, 71883, 71884, 71885, 71886, 71887,
                        71888, 71889, 71890, 71891, 71892, 71893, 71894, 71895, 71896, 71897, 71898, 71899, 71900,
                        71901, 71902, 71903, 93792, 93793, 93794, 93795, 93796, 93797, 93798, 93799, 93800, 93801,
                        93802, 93803, 93804, 93805, 93806, 93807, 93808, 93809, 93810, 93811, 93812, 93813, 93814,
                        93815, 93816, 93817, 93818, 93819, 93820, 93821, 93822, 93823, 125218, 125219, 125220, 125221,
                        125222, 125223, 125224, 125225, 125226, 125227, 125228, 125229, 125230, 125231, 125232, 125233,
                        125234, 125235, 125236, 125237, 125238, 125239, 125240, 125241, 125242, 125243, 125244, 125245,
                        125246, 125247, 125248, 125249, 125250, 125251];

for (let iter of lower_expected) {
  let cp = String.fromCodePoint(iter);
  assert(cp !== cp.toLowerCase());
}

for (let iter of upper_expected) {
  let cp = String.fromCodePoint(iter);
  assert(cp !== cp.toUpperCase());
}

assert("\ud801A".toLowerCase() === "\ud801a");
