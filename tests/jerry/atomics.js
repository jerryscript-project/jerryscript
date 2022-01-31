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
const uint8 = new Uint8Array (buffer);
uint8[0] = 7;

Atomics.add (uint8, 0, 2);
Atomics.and (uint8, 0, 2);
Atomics.compareExchange (uint8, 0, 5, 2);
Atomics.exchange (uint8, 0, 2);
Atomics.or (uint8, 0, 2);
Atomics.sub (uint8, 0, 2);
Atomics.xor (uint8, 0, 2)
Atomics.isLockFree (3);
Atomics.load (uint8, 0);
Atomics.store (uint8, 0, 2);

const sab = new SharedArrayBuffer (1024);
const int32 = new Int32Array (sab);

Atomics.wait (int32, 0, 0);
Atomics.notify (int32, 0, 1);

try {
  let a;
  Atomics.add (a, 0, 0);
} catch (ex) {
  assert (ex instanceof TypeError);
}

try {
  Atomics.add (new Float32Array(10), 0, 2);
} catch (ex) {
  assert (ex instanceof TypeError);
}
try{
  const uint16 = new Uint16Array (new ArrayBuffer (16));
  Atomics.add(uint16, 0, 0);
} catch (ex) {
  assert (ex instanceof TypeError);
}

try {
  Atomics.add (uint8, 100, 0);
} catch (ex) {
  assert (ex instanceof RangeError);
}

try {
  Atomics.add (uint8, -1, 0);
} catch (ex) {
  assert (ex instanceof RangeError);
}
