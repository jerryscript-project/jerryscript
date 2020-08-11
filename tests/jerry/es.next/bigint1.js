/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

function check_result(bigint, expected)
{
  assert(bigint.toString() === expected)
}

function check_result16(bigint, expected)
{
  assert(bigint.toString(16) === expected)
}

function check_syntax_error(code)
{
  try {
    eval(code)
    assert(false)
  } catch (e) {
    assert(e instanceof SyntaxError)
  }
}

assert(typeof BigInt("0") == "bigint")

// Test BigInt string parsing and toString

check_syntax_error("BigInt('-0x5')");
check_syntax_error("BigInt('-')");
check_syntax_error("BigInt('00x5')");
check_syntax_error("BigInt('11a')");
check_syntax_error("BigInt('0b2')");
check_syntax_error("BigInt('1n')");

check_result(BigInt("0"), "0")
check_result(BigInt("-0"), "0")
check_result(BigInt("100000000000000000000000000000000000000"), "100000000000000000000000000000000000000")
check_result(BigInt("-1234567890123456789012345678901234567890"), "-1234567890123456789012345678901234567890")
check_result(BigInt("+1"), "1")
check_result(BigInt("+000000000000000000001"), "1")
check_result(BigInt("-000000000000000000000"), "0")
check_result(BigInt("0x00abcdefABCDEF0123456789000000000000000"), "239460437713606077082343926293727858623774720")
check_result(BigInt("0b00100000000000010000000000010000000000010"), "274911469570")

assert(BigInt("100000000000000000000000000000000000000").toString(22) === "2ci67fiek1bkhec5fig7aiii9hf8c")
check_result16(BigInt("239460437713606077082343926293727858623774720"), "abcdefabcdef0123456789000000000000000")

// Test negate

check_result(-BigInt("0"), "0")
check_result(-BigInt("100"), "-100")
check_result(-BigInt("-100"), "100")
check_result(-BigInt("100000000000000000000000000000000000000000000"), "-100000000000000000000000000000000000000000000")
check_result(-BigInt("-100000000000000000000000000000000000000000000"), "100000000000000000000000000000000000000000000")

// Test addition

check_result(BigInt("0") + BigInt("0"), "0")
check_result(BigInt("1") + BigInt("1"), "2")
check_result(BigInt("0") + BigInt("100"), "100")
check_result(BigInt("0") + BigInt("-100"), "-100")
check_result(BigInt("100") + BigInt("0"), "100")
check_result(BigInt("-100") + BigInt("0"), "-100")

check_result(BigInt("100000000000000000000000000000000000000") + BigInt("100000000000000000000000000000000000000"),
             "200000000000000000000000000000000000000");
check_result(BigInt("-100000000000000000000000000000000000000") + BigInt("-100000000000000000000000000000000000000"),
             "-200000000000000000000000000000000000000");
check_result(BigInt("0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff3") + BigInt("0xd"),
             "115792089237316195423570985008687907853269984665640564039457584007913129639936");
check_result(BigInt("0xd") + BigInt("0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff3"),
             "115792089237316195423570985008687907853269984665640564039457584007913129639936");

check_result(BigInt("100000000000000000000000000000000000000") + BigInt("-100000000000000000000000000000000000000"), "0")
check_result(BigInt("100000000000000000000000000000000000001") + BigInt("-100000000000000000000000000000000000000"), "1")
check_result(BigInt("100000000000000000000000000000000000000") + BigInt("-100000000000000000000000000000000000001"), "-1")
check_result(BigInt("-100000000000000000000000000000000000000") + BigInt("100000000000000000000000000000000000000"), "0")
check_result(BigInt("-100000000000000000000000000000000000001") + BigInt("100000000000000000000000000000000000000"), "-1")
check_result(BigInt("-100000000000000000000000000000000000000") + BigInt("100000000000000000000000000000000000001"), "1")

// Test substraction

check_result(BigInt("0") - BigInt("0"), "0")
check_result(BigInt("2") - BigInt("1"), "1")
check_result(BigInt("0") - BigInt("100"), "-100")
check_result(BigInt("0") - BigInt("-100"), "100")
check_result(BigInt("100") - BigInt("0"), "100")
check_result(BigInt("-100") - BigInt("0"), "-100")

check_result(BigInt("100000000000000000000000000000000000000") - BigInt("-100000000000000000000000000000000000000"),
             "200000000000000000000000000000000000000");
check_result(BigInt("-100000000000000000000000000000000000000") - BigInt("100000000000000000000000000000000000000"),
             "-200000000000000000000000000000000000000");
check_result(BigInt("100000000000000000000000000000000000000") - BigInt("-1"),
             "100000000000000000000000000000000000001");
check_result(BigInt("-100000000000000000000000000000000000000") - BigInt("1"),
             "-100000000000000000000000000000000000001");
check_result(BigInt("1") - BigInt("-100000000000000000000000000000000000000"),
             "100000000000000000000000000000000000001");
check_result(BigInt("-1") - BigInt("100000000000000000000000000000000000000"),
             "-100000000000000000000000000000000000001");

check_result(BigInt("100000000000000000000000000000000000000") - BigInt("100000000000000000000000000000000000000"), "0")
check_result(BigInt("100000000000000000000000000000000000001") - BigInt("100000000000000000000000000000000000000"), "1")
check_result(BigInt("100000000000000000000000000000000000000") - BigInt("100000000000000000000000000000000000001"), "-1")
check_result(BigInt("-100000000000000000000000000000000000000") - BigInt("-100000000000000000000000000000000000000"), "0")
check_result(BigInt("-100000000000000000000000000000000000001") - BigInt("-100000000000000000000000000000000000000"), "-1")
check_result(BigInt("-100000000000000000000000000000000000000") - BigInt("-100000000000000000000000000000000000001"), "1")

// Test multiplication

check_result(BigInt("0") * BigInt("0"), "0")
check_result(BigInt("1000") * BigInt("0"), "0")
check_result(BigInt("0") * BigInt("1000"), "0")
check_result(BigInt("1") * BigInt("100000000000000000000000000000000000000"), "100000000000000000000000000000000000000")
check_result(BigInt("1") * BigInt("-100000000000000000000000000000000000000"), "-100000000000000000000000000000000000000")
check_result(BigInt("-1") * BigInt("100000000000000000000000000000000000000"), "-100000000000000000000000000000000000000")
check_result(BigInt("-1") * BigInt("-100000000000000000000000000000000000000"), "100000000000000000000000000000000000000")
check_result(BigInt("100000000000000000000000000000000000000") * BigInt("1"), "100000000000000000000000000000000000000")
check_result(BigInt("-100000000000000000000000000000000000000") * BigInt("1"), "-100000000000000000000000000000000000000")
check_result(BigInt("100000000000000000000000000000000000000") * BigInt("-1"), "-100000000000000000000000000000000000000")
check_result(BigInt("-100000000000000000000000000000000000000") * BigInt("-1"), "100000000000000000000000000000000000000")

check_result(BigInt("100000000000000000000000000000000000000") * BigInt("100000000000000000000000000000000000000"),
             "10000000000000000000000000000000000000000000000000000000000000000000000000000")
check_result(BigInt("100000000000000000000000000000000000000") * BigInt("-100000000000000000000000000000000000000"),
             "-10000000000000000000000000000000000000000000000000000000000000000000000000000")
check_result(BigInt("-100000000000000000000000000000000000000") * BigInt("100000000000000000000000000000000000000"),
             "-10000000000000000000000000000000000000000000000000000000000000000000000000000")
check_result(BigInt("-100000000000000000000000000000000000000") * BigInt("-100000000000000000000000000000000000000"),
             "10000000000000000000000000000000000000000000000000000000000000000000000000000")

// Test divide

try {
  BigInt("32") / BigInt("0")
  assert(false)
} catch (e) {
  assert(e instanceof RangeError)
}

try {
  BigInt("32") % BigInt("0")
  assert(false)
} catch (e) {
  assert(e instanceof RangeError)
}

check_result(BigInt("0") / BigInt("1234"), "0")
check_result(BigInt("0") % BigInt("1234"), "0")

check_result(BigInt("100") / BigInt("70"), "1")
check_result(BigInt("100") % BigInt("70"), "30")
check_result(BigInt("-100") / BigInt("70"), "-1")
check_result(BigInt("-100") % BigInt("70"), "-30")
check_result(BigInt("100") / BigInt("-70"), "-1")
check_result(BigInt("100") % BigInt("-70"), "30")
check_result(BigInt("-100") / BigInt("-70"), "1")
check_result(BigInt("-100") % BigInt("-70"), "-30")

check_result(BigInt("100") / BigInt("100"), "1")
check_result(BigInt("100") % BigInt("100"), "0")
check_result(BigInt("-100") / BigInt("100"), "-1")
check_result(BigInt("-100") % BigInt("100"), "0")
check_result(BigInt("100") / BigInt("-100"), "-1")
check_result(BigInt("100") % BigInt("-100"), "0")
check_result(BigInt("-100") / BigInt("-100"), "1")
check_result(BigInt("-100") % BigInt("-100"), "0")

/* Division by small value. */
check_result(BigInt("100000000000000000000") / BigInt("1000000"), "100000000000000")
check_result(BigInt("100000000000000000000") % BigInt("1000000"), "0")
check_result(BigInt("12345678901234567890") / BigInt("1000000"), "12345678901234")
check_result(BigInt("12345678901234567890") % BigInt("1000000"), "567890")

/* Division by large value. */
check_result(BigInt("100000000000000000000") / BigInt("100000000000000000"), "1000")
check_result(BigInt("100000000000000000000") % BigInt("100000000000000000"), "0")
check_result(BigInt("12345678901234567890123456789012345678901234567890123456789012345678901234567890") / BigInt("10000000000000000000000000000000000000"),
                    "1234567890123456789012345678901234567890123")
check_result(BigInt("12345678901234567890123456789012345678901234567890123456789012345678901234567890") % BigInt("10000000000000000000000000000000000000"),
                    "4567890123456789012345678901234567890")
check_result16(BigInt("0xffffffffffffffffffffffff") / BigInt("0x100000000"), "ffffffffffffffff")
check_result16(BigInt("0xffffffffffffffffffffffff") % BigInt("0x100000000"), "ffffffff")

/* Triggers a corner case. */
check_result(BigInt("170141183420855150493001878992821682176") / BigInt("39614081266355540842216685573"), "4294967293")
check_result(BigInt("170141183420855150493001878992821682176") % BigInt("39614081266355540842216685573"), "39614081266355540837921718287")

// Test shift

check_result(BigInt("0") << BigInt("10000000"), "0")
check_result(BigInt("0") >> BigInt("10000000"), "0")
check_result(BigInt("10000000") << BigInt("0"), "10000000")
check_result(BigInt("10000000") >> BigInt("0"), "10000000")

check_result(BigInt("4096") << BigInt("2"), "16384")
check_result(BigInt("4096") << BigInt("-2"), "1024")
check_result(BigInt("4096") >> BigInt("2"), "1024")
check_result(BigInt("4096") >> BigInt("-2"), "16384")

check_result16(BigInt("0x8fef5fcfffef5fcfffef5fcfffef5fcff") << BigInt("1"), "11fdebf9fffdebf9fffdebf9fffdebf9fe")
check_result16(BigInt("0x8fef5fcfffef5fcfffef5fcfffef5fcff") << BigInt("19"), "47f7afe7fff7afe7fff7afe7fff7afe7f80000")
check_result16(BigInt("0x8fef5fcfffef5fcfffef5fcfffef5fcff") << BigInt("31"), "47f7afe7fff7afe7fff7afe7fff7afe7f80000000")
check_result16(BigInt("0x8fef5fcfffef5fcfffef5fcfffef5fcff") << BigInt("32"), "8fef5fcfffef5fcfffef5fcfffef5fcff00000000")
check_result16(BigInt("0x8fef5fcfffef5fcfffef5fcfffef5fcff") << BigInt("51"), "47f7afe7fff7afe7fff7afe7fff7afe7f8000000000000")
check_result16(BigInt("0x8fef5fcfffef5fcfffef5fcfffef5fcff") << BigInt("63"), "47f7afe7fff7afe7fff7afe7fff7afe7f8000000000000000")
check_result16(BigInt("0x8fef5fcfffef5fcfffef5fcfffef5fcff") << BigInt("64"), "8fef5fcfffef5fcfffef5fcfffef5fcff0000000000000000")

check_result16(BigInt("0x8fef5fcfffef5fcfffef5fcfffef5fcff") >> BigInt("1"), "47f7afe7fff7afe7fff7afe7fff7afe7f")
check_result16(BigInt("0x8fef5fcfffef5fcfffef5fcfffef5fcff") >> BigInt("19"), "11fdebf9fffdebf9fffdebf9fffde")
check_result16(BigInt("0x8fef5fcfffef5fcfffef5fcfffef5fcff") >> BigInt("31"), "11fdebf9fffdebf9fffdebf9ff")
check_result16(BigInt("0x8fef5fcfffef5fcfffef5fcfffef5fcff") >> BigInt("32"), "8fef5fcfffef5fcfffef5fcff")
check_result16(BigInt("0x8fef5fcfffef5fcfffef5fcfffef5fcff") >> BigInt("51"), "11fdebf9fffdebf9fffde")
check_result16(BigInt("0x8fef5fcfffef5fcfffef5fcfffef5fcff") >> BigInt("63"), "11fdebf9fffdebf9ff")
check_result16(BigInt("0x8fef5fcfffef5fcfffef5fcfffef5fcff") >> BigInt("64"), "8fef5fcfffef5fcff")

check_result16(-BigInt("0xff") >> BigInt("8"), "-1")
check_result16(-BigInt("0xff") >> BigInt("1000"), "-1")
check_result16(-BigInt("0xff") >> BigInt("7"), "-2")
check_result16(-BigInt("0xff00000000") >> BigInt("32"), "-ff")
check_result16(-BigInt("0xff80000000") >> BigInt("32"), "-100")
check_result16(-BigInt("0xff00000000000000000000000000000000") >> BigInt("128"), "-ff")
check_result16(-BigInt("0xff80000000000000000000000000000000") >> BigInt("128"), "-100")
check_result16(-BigInt("0xfe00000000000000000000000000000000") >> BigInt("129"), "-7f")
check_result16(-BigInt("0xff00000000000000000000000000000000") >> BigInt("129"), "-80")

check_result16(BigInt("0x8fef5fcfffef5fcfffef5fcfffef5fcff") >> BigInt("10000000000000000000000000"), "0")

try {
  BigInt("0x8fef5fcfffef5fcfffef5fcfffef5fcff") << BigInt("10000000000000000000000000");
  assert(false)
} catch (e) {
  assert(e instanceof RangeError)
}
