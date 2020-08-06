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

/* Boolean. */

assert(!BigInt("0") === true)
assert(!BigInt("1") === false)
assert(!BigInt("-1") === false)

/* Strict equal. */

assert(BigInt("0") === BigInt("0"))
assert(BigInt("-77") === BigInt("-77"))
assert(!(BigInt("-77") !== BigInt("-77")))

assert(!(Object(BigInt("-77")) === BigInt("-77")))
assert(BigInt("-77") !== Object(BigInt("-77")))

assert(BigInt("0xffffffffffffffffffffffffffffffff") === BigInt("0xffffffffffffffffffffffffffffffff"))
assert(!(BigInt("0xfffffffffffffffffffffffffffffffe") === BigInt("0xffffffffffffffffffffffffffffffff")))
assert(BigInt("0x100000000000000000000000000000000") !== BigInt("0xffffffffffffffffffffffffffffffff"))
assert(!(BigInt("1234") === 1234))
assert(-4567 !== BigInt("-4567"))

/* Equal. */

assert(BigInt("0x100") == BigInt("256"))
assert(BigInt("-77") == "-77")
assert("168" == BigInt("168"))
assert(!("0xffffffffffffffffffffffffffffffff" != BigInt("0xffffffffffffffffffffffffffffffff")))

assert(BigInt("0x1000") == 0x1000)
assert(468123 == BigInt("468123"))

assert(!(BigInt("100000000") == 100000000.5))
assert(-0.125 != BigInt("0"))
assert(!("InvalidBigIntString" == BigInt("100000000")))
assert(BigInt("100000000") != "10000 0000")

assert(BigInt("0") == 0)
assert(!(-0 != BigInt("0")))
assert(!(BigInt("0") == 0.0000152587890625))
assert(0.0000152587890625 != BigInt("0"))

assert(!(BigInt("100000000") == NaN))
assert(NaN != BigInt("-100000000"))
assert(!(BigInt("100000000000000000000000000") == Infinity))
assert(Infinity != BigInt("100000000000000000000000000"))

/* Relational. */

assert(!(BigInt("1234") < BigInt("1234")))
assert(BigInt("1234") >= BigInt("1234"))
assert(BigInt("1234") <= BigInt("1234"))
assert(!(BigInt("1234") > BigInt("1234")))

assert(BigInt("1234") < BigInt("1235"))
assert(!(BigInt("1234") >= BigInt("1235")))
assert(BigInt("1234") <= BigInt("1235"))
assert(!(BigInt("1234") > BigInt("1235")))

assert(!(BigInt("123456789012345678901234567890") < "123456789012345678901234567890"))
assert(BigInt("123456789012345678901234567890") >= "123456789012345678901234567890")
assert(BigInt("123456789012345678901234567890") <= "123456789012345678901234567890")
assert(!(BigInt("123456789012345678901234567890") > "123456789012345678901234567890"))

assert(!("0x1234567890abcdef1234567890abcdef" < BigInt("0x1234567890abcdef1234567890abcdef")))
assert("0x1234567890abcdef1234567890abcdef" >= BigInt("0x1234567890abcdef1234567890abcdef"))
assert("0x1234567890abcdef1234567890abcdef" <= BigInt("0x1234567890abcdef1234567890abcdef"))
assert(!("0x1234567890abcdef1234567890abcdef" > BigInt("0x1234567890abcdef1234567890abcdef")))

assert(!("Invalid" < BigInt("100")))
assert(!("Invalid" >= BigInt("100")))
assert(!("Invalid" <= BigInt("100")))
assert(!("Invalid" > BigInt("100")))

assert(!(BigInt("0") < "NotABigInt"))
assert(!(BigInt("0") >= "NotABigInt"))
assert(!(BigInt("0") <= "NotABigInt"))
assert(!(BigInt("0") > "NotABigInt"))

assert(!(BigInt("0") < 0))
assert(BigInt("0") >= 0)
assert(BigInt("0") <= 0)
assert(!(BigInt("0") > 0))

assert(!(-0 < BigInt("0")))
assert(-0 >= BigInt("0"))
assert(-0 <= BigInt("0"))
assert(!(-0 > BigInt("0")))

assert(BigInt("0") < 67)
assert(!(BigInt("0") > 67))
assert(!(BigInt("0") < -0.125))
assert(BigInt("0") > -0.125)

assert(!(BigInt("7") < NaN))
assert(!(BigInt("7") >= NaN))
assert(!(BigInt("7") <= NaN))
assert(!(BigInt("7") > NaN))

assert(!(Infinity < BigInt("1000000000000000000000000000000")))
assert(!(Infinity <= BigInt("1000000000000000000000000000000")))
assert(Infinity >= BigInt("1000000000000000000000000000000"))
assert(Infinity > BigInt("1000000000000000000000000000000"))

assert(-Infinity < BigInt("1000000000000000000000000000000"))
assert(-Infinity <= BigInt("1000000000000000000000000000000"))
assert(!(-Infinity >= BigInt("1000000000000000000000000000000")))
assert(!(-Infinity > BigInt("1000000000000000000000000000000")))

assert(BigInt("-10000") < 1)
assert(BigInt("-10000") <= 1)
assert(!(BigInt("-10000") >= 1))
assert(!(BigInt("-10000") > 1))

assert(!(1 < BigInt("-12345678")))
assert(!(1 <= BigInt("-12345678")))
assert(1 >= BigInt("-12345678"))
assert(1 > BigInt("-12345678"))

assert(!(BigInt("1") < 0.5))
assert(!(BigInt("1") <= 0.5))
assert(BigInt("1") >= 0.5)
assert(BigInt("1") > 0.5)

assert(!(-0.5 < BigInt("-1")))
assert(!(-0.5 <= BigInt("-1")))
assert(-0.5 >= BigInt("-1"))
assert(-0.5 > BigInt("-1"))

assert(!(BigInt("0x1000000000000000000000000000000") < 0x100000))
assert(!(BigInt("0x1000000000000000000000000000000") <= 0x100000))
assert(BigInt("0x1000000000000000000000000000000") >= 0x100000)
assert(BigInt("0x1000000000000000000000000000000") > 0x100000)

assert(-0x1000000000000000000000000000000 < BigInt("-1234"))
assert(-0x1000000000000000000000000000000 <= BigInt("-1234"))
assert(!(-0x1000000000000000000000000000000 >= BigInt("-1234")))
assert(!(-0x1000000000000000000000000000000 > BigInt("-1234")))

assert(0x1234567880000000000000000000000 < BigInt("0x1234567890000000000000000000000"))
assert(0x1234567880000000000000000000000 <= BigInt("0x1234567890000000000000000000000"))
assert(!(0x1234567880000000000000000000000 > BigInt("0x1234567890000000000000000000000")))
assert(!(0x1234567880000000000000000000000 >= BigInt("0x1234567890000000000000000000000")))

assert(-BigInt("0x1234567890000000000000000000000") < -0x1234567880000000000000000000000)
assert(-BigInt("0x1234567890000000000000000000000") <= -0x1234567880000000000000000000000)
assert(!(-BigInt("0x1234567890000000000000000000000") >= -0x1234567880000000000000000000000))
assert(!(-BigInt("0x1234567890000000000000000000000") > -0x1234567880000000000000000000000))

// True because of rounding
assert(0x1234567890000000000000000000001 < BigInt("0x1234567890000000000000000000001"))
assert(0x1234567890000000000000000000001 <= BigInt("0x1234567890000000000000000000001"))
assert(!(0x1234567890000000000000000000001 >= BigInt("0x1234567890000000000000000000001")))
assert(!(0x1234567890000000000000000000001 > BigInt("0x1234567890000000000000000000001")))

assert(-BigInt("0x1234567890000000000000000000001") < -0x1234567890000000000000000000001)
assert(-BigInt("0x1234567890000000000000000000001") <= -0x1234567890000000000000000000001)
assert(!(-BigInt("0x1234567890000000000000000000001") >= -0x1234567890000000000000000000001))
assert(!(-BigInt("0x1234567890000000000000000000001") > -0x1234567890000000000000000000001))

assert(!(1.0000152587890625 < BigInt("1")))
assert(!(1.0000152587890625 <= BigInt("1")))
assert(1.0000152587890625 >= BigInt("1"))
assert(1.0000152587890625 > BigInt("1"))

assert(!(BigInt("-1") < -1.0000152587890625))
assert(!(BigInt("-1") <= -1.0000152587890625))
assert(BigInt("-1") >= -1.0000152587890625)
assert(BigInt("-1") > -1.0000152587890625)
