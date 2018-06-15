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

var a = new Int32Array([1, 2, 3, 4, 5]);
var b = new Int32Array(5);

try {
  a.set(b, 123456);
  assert(1 === 0); // Should not get here.
} catch (e) {
  assert(e instanceof RangeError);
}

b.set(a);
assert(b.join() === '1,2,3,4,5');
try {
  b.set(a, 1);
  assert(1 === 0); // Should not get here.
} catch (e) {
  assert(e instanceof RangeError);
}

b.set(new Int32Array([99, 98]), 2);
assert(b.join() === '1,2,99,98,5');

b.set(new Int32Array([99, 98, 97]), 2);
assert(b.join() === '1,2,99,98,97');

try {
  b.set(new Int32Array([99, 98, 97, 96]), 2);
  assert(1 === 0); // Should not get here.
} catch (e) {
  assert(e instanceof RangeError);
}

try {
  b.set([101, 102, 103, 104], 4);
  assert(1 === 0); // Should not get here.
} catch (e) {
  assert(e instanceof RangeError);
}

//  ab = [ 0, 1, 2, 3, 4, 5, 6, 7 ]
//  a1 = [ ^, ^, ^, ^, ^, ^, ^, ^ ]
//  a2 =             [ ^, ^, ^, ^ ]
var ab = new ArrayBuffer(8);
var a1 = new Uint8Array(ab);
for (var i = 0; i < a1.length; i += 1) {
  a1.set([i], i);
}

var a2 = new Uint8Array(ab, 4);
a1.set(a2, 2);
assert(a1.join() === '0,1,4,5,6,7,6,7');
assert(a2.join() === '6,7,6,7');

var a3 = new Uint32Array(ab, 4);
a1.set(a3, 2);
assert(a1.join() === '0,1,6,5,6,7,6,7');
assert(a3.join() === '117835526');

var a4 = new Uint8Array(ab, 0, 4);
a1.set(a4, 2);
assert(a1.join() === '0,1,0,1,6,5,6,7');
assert(a4.join() === '0,1,0,1');

var a5 = new Uint32Array(ab, 4, 1);
a1.set(a5, 2);
assert(a1.join() === '0,1,6,1,6,5,6,7');
assert(a5.join() === '117835014');

var c = new Int32Array([0xFFFFFFFF]);
var d = new Uint8Array(4);
d.set(c);
assert(d.join() === '255,0,0,0');

var e = new Float32Array([3.33]);
var f = new Uint8Array(1);
f.set(e);
assert(f.join() === '3');
e.set(f);
assert(e.join() === '3');
