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

const buffer = new SharedArrayBuffer (16);
const uint8 = new Uint8Array(buffer);

Atomics.isLockFree(3);

const sab = new SharedArrayBuffer (1024);
const int32 = new Int32Array (sab);

Atomics.wait (int32, 0, 0);
Atomics.notify (int32, 0, 1);

try {
  let a;
  Atomics.add (a, 0, 0);
  assert(false);
} catch (ex) {
  assert (ex instanceof TypeError);
}

try {
  Atomics.add (new Float32Array(10), 0, 2);
  assert(false);
} catch (ex) {
  assert (ex instanceof TypeError);
}

try {
  Atomics.add (uint8, 100, 0);
  assert(false);
} catch (ex) {
  assert (ex instanceof RangeError);
}

try {
  Atomics.add (uint8, -1, 0);
  assert(false);
} catch (ex) {
  assert (ex instanceof RangeError);
}

assert(Atomics.store(uint8, 0, -1) === -1);
assert(Atomics.add(uint8, 0, 1) === 255);
assert(Atomics.load(uint8, 0) === 0);

const ta_int8 = new Int8Array(new ArrayBuffer(2));
ta_int8[0] = 5;

Atomics.store(ta_int8, 1, 127)

assert(Atomics.load(ta_int8, 0) === 5);

assert(Atomics.add(ta_int8, 0, 12) === 5);
assert(Atomics.load(ta_int8, 0) === 17);

assert(Atomics.and(ta_int8, 0, 1) === 17);
assert(Atomics.load(ta_int8, 0) === 1);

assert(Atomics.compareExchange(ta_int8, 0, 5, 123) === 1);
assert(Atomics.load(ta_int8, 0) === 1);

assert(Atomics.compareExchange(ta_int8, 1, 123, ta_int8[0]) === 127);
assert(Atomics.load(ta_int8, 0) === 1);

assert(Atomics.compareExchange(ta_int8, 0, 1, 123) === 1);
assert(Atomics.load(ta_int8, 0) === 123);

assert(Atomics.store(ta_int8, 0, -0) === 0);
assert(Atomics.load(ta_int8, 0) === 0);

assert(Atomics.store(ta_int8, 0, 1) === 1);
assert(Atomics.load(ta_int8, 0) === 1);

assert(Atomics.exchange(ta_int8, 0, 12) === 1);
assert(Atomics.load(ta_int8, 0) === 12);

assert(Atomics.or(ta_int8, 0, 1) === 12);
assert(Atomics.load(ta_int8, 0) === 13);

assert(Atomics.sub(ta_int8, 0, 2) === 13);
assert(Atomics.load(ta_int8, 0) === 11);

assert(Atomics.xor(ta_int8, 0, 1) === 11);
assert(Atomics.load(ta_int8, 0) === 10);

const ta_bigintint64 = new BigInt64Array(new ArrayBuffer(16));
ta_bigintint64[0] = 5n;

Atomics.store(ta_bigintint64, 1, 127n)

assert(Atomics.load(ta_bigintint64, 0) === 5n);

assert(Atomics.add(ta_bigintint64, 0, 12n) === 5n);
assert(Atomics.load(ta_bigintint64, 0) === 17n);

assert(Atomics.and(ta_bigintint64, 0, 1n) === 17n);
assert(Atomics.load(ta_bigintint64, 0) === 1n);

assert(Atomics.compareExchange(ta_bigintint64, 0, 5n, 123n) === 1n);
assert(Atomics.load(ta_bigintint64, 0) === 1n);

assert(Atomics.compareExchange(ta_bigintint64, 1, 123n, ta_bigintint64[0]) === 127n);
assert(Atomics.load(ta_bigintint64, 0) === 1n);

assert(Atomics.compareExchange(ta_bigintint64, 0, 1n, 123n) === 1n);
assert(Atomics.load(ta_bigintint64, 0) === 123n);

assert(Atomics.store(ta_bigintint64, 0, -0n) === 0n);
assert(Atomics.load(ta_bigintint64, 0) === 0n);

assert(Atomics.store(ta_bigintint64, 0, 1n) === 1n);
assert(Atomics.load(ta_bigintint64, 0) === 1n);

assert(Atomics.exchange(ta_bigintint64, 0, 12n) === 1n);
assert(Atomics.load(ta_bigintint64, 0) === 12n);

assert(Atomics.or(ta_bigintint64, 0, 1n) === 12n);
assert(Atomics.load(ta_bigintint64, 0) === 13n);

assert(Atomics.sub(ta_bigintint64, 0, 2n) === 13n);
assert(Atomics.load(ta_bigintint64, 0) === 11n);

assert(Atomics.xor(ta_bigintint64, 0, 1n) === 11n);
assert(Atomics.load(ta_bigintint64, 0) === 10n);

try {
  Atomics.store(ta_bigintint64, 0, 1);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  Atomics.store(ta_int8, 0, 1n);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  Atomics.store(ta_int8, 0n, 1);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  Atomics.store(ta_int8, 2, 1);
  assert(false);
} catch (e) {
  assert(e instanceof RangeError);
}

try {
  Atomics.store(ta_int8, -1, 1);
  assert(false);
} catch (e) {
  assert(e instanceof RangeError);
}

try {
  Atomics.load(ta_int8, 0n);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  Atomics.load(ta_int8, 2);
  assert(false);
} catch (e) {
  assert(e instanceof RangeError);
}

try {
  Atomics.load(ta_int8, -1);
  assert(false);
} catch (e) {
  assert(e instanceof RangeError);
}

try {
  Atomics.exchange(ta_bigintint64, 0, 1);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  Atomics.exchange(ta_int8, 0, 1n);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  Atomics.exchange(ta_int8, 0n, 1);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  Atomics.compareExchange(ta_bigintint64, 0n, 1n, 0n);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  Atomics.compareExchange(ta_bigintint64, 0, 1, 0n);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  Atomics.compareExchange(ta_int8, 0, 1n, 0);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  Atomics.compareExchange(ta_bigintint64, 0, 1n, 0);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  Atomics.compareExchange(ta_int8, 0, 1, 0n);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  Atomics.compareExchange(ta_int8, 2, 1, 0);
  assert(false);
} catch (e) {
  assert(e instanceof RangeError);
}

try {
  Atomics.compareExchange(ta_int8, -1, 1, 0);
  assert(false);
} catch (e) {
  assert(e instanceof RangeError);
}

const ta_float32 = new Float32Array(new ArrayBuffer(4));

try {
  Atomics.store(ta_float32, 0, 127);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  Atomics.load(ta_float32, 0);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

try {
  Atomics.compareExchange(ta_float32, 0, 1, 0);
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}
