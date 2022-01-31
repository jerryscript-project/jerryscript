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

// Test bitwise 'and' operation

assert((0n & 0n) === 0n)
assert((0x12345678n & 0n) === 0n)
assert((0n & 0x12345678n) === 0n)
assert((0xff00ff00ff00ff00ff00n & 0xff00ff00ff00ff00ffn) === 0n)
assert((0x12345678ffffffff12345678ffffffffn & 0xffff87654321n) === 0x567887654321n)
assert((0xf56cd2479efcdn & 0x56cdf23bc02134e3bdc56f43297be4c27n) === 0x6540004384c05n)
assert((0x45c308bd83cf279n & -0x100000000n) === 0x45c308b00000000n)
assert((-0x10000000000000000n & 0xffffffffffffffffn) === 0n)
assert((-0x11234567890abcdefn & 0xffffffffffffffffffffn) === 0xfffeedcba9876f543211n)
assert((-0x10000000000000n & -0x10000000000000n) === -0x10000000000000n)
assert((-0x100000000000000001n & -0x1n) === -0x100000000000000001n)

// Test bitwise 'or' operation

assert((0n | 0n) === 0n)
assert((0x123456789abcdefn | 0n) === 0x123456789abcdefn)
assert((0n | 0x123456789abcdefn) === 0x123456789abcdefn)
assert((0xaa00bb00cc00dd00ee00n | 0xff00ee00dd00cc00bbn) === 0xaaffbbeeccddddcceebbn);
assert((0xfedcba09876543210fedcba09876543210n | 0x7n) === 0xfedcba09876543210fedcba09876543217n)
assert((0x8n | 0xfedcba09876543210fedcba09876543210n) === 0xfedcba09876543210fedcba09876543218n)
assert((-0xc34bd5f946c7a92b69b3a96cd7c2a12n | 0xfcbacfbn) === -0xc34bd5f946c7a92b69b3a96c0340201n)
assert((-0xb314c297ba3n | 0xfeacb00000000n) === -0x1304c297ba3n)
assert((-0x74b186cd308b377cb23n | -0x5cba7935b213cd657d937c42975de63802a7b92cd49an) === -0x74900280200b124c001n)
assert((-0x10000000000000000n | -0x100000000000000000000000000000000n) === -0x10000000000000000n)

// Test bitwise 'xor' operation

assert((0n ^ 0n) === 0n)
assert((0x123456789abcdefn ^ 0n) === 0x123456789abcdefn)
assert((0n ^ 0x123456789abcdefn) === 0x123456789abcdefn)
assert((0x74b186cd308b355cb23cd28cd75n ^ 0x74b186cd308b355cb23cd28cd75n) === 0n)
assert((0xff00ff00ff00ff00ff00ff00ff00ff00ff00ff00ffn ^ 0xf0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0fn) === 0xff00ff00ff00ff00ff00ff00ff00ff00ff00ff00ff0n)
assert((0x31988644a57e18n ^ 0xb2303b6f2efcb4de7761c01622440f9d985d07dbfe03c9f1n) === 0xb2303b6f2efcb4de7761c01622440f9d986c9f5dbaa6b7e9n)
assert((-0xd858541b8eb3e6ae247b1f84dbd8cc2db66n ^ 0x0811a0e70710fcf965n) === -0xd858541b8eb3e6ae24fa058aaba9c3e2201n)
assert((0x38d00faa3f33n ^ -0x89c40cdc4a064dcd8b3663feb322026dn) === -0x89c40cdc4a064dcd8b365b2ebc883d60n)
assert((-0x66cb3001b88361a25b8715922n ^ -0x66cb3001b88361a25b8715922n) === 0n)
assert((-0x893bff556397300afe6411d8727c0aaffn ^ -0xef69f24dfcd1447397d62217c6ad2n) === 0x893b103c91daccdbba17860e506bcc02fn)
