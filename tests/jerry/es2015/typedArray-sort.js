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

// Default sorting behavior.
var a = Uint8Array.from([4, 1, 3, 5, 4, 2]);
assert(a.sort().toString() === '1,2,3,4,4,5');
assert(a.toString() === '1,2,3,4,4,5');

// Views into typedarrays should be properly sorted.
var b = Uint8Array.from([2, 1, 4, 3, 0]);
assert(b.subarray(2, 4).sort().toString() === '3,4');
assert(b.toString() === '2,1,3,4,0');

// Empty typedarrays should be able to be "sorted".
var c = Uint8Array.from([]);
assert(c.sort().toString() === '');

// Infinity should be supported.
var d = Float32Array.from([Infinity, 3, 2, 1, -Infinity]);
assert(d.sort().toString() === '-Infinity,1,2,3,Infinity');

// +0 and -0 should be properly sorted.
var e = Float32Array.from([1, 0, -0, -1]);
assert(e.sort().toString() === '-1,0,0,1');

// NaN should be supported and always at the end.
var f = Float32Array.from([NaN, 0, 1, -1, -Infinity, Infinity, NaN]);
assert(f.sort().toString() === '-Infinity,-1,0,1,Infinity,NaN,NaN');

// The element size of the view should be sorted properly.
var ab = new ArrayBuffer(4);
var g = new Uint32Array(ab);
var h = new Uint8Array(ab);
h.set([0xFF, 0, 0xFF, 0]);
assert(h.toString() === '255,0,255,0');
assert(g.toString() === '16711935');
assert(h.subarray(0, 2).sort().toString() === '0,255');
assert(h.subarray(2, 4).sort().toString() === '0,255');
assert(g.toString() === '4278255360');
assert(g.sort().toString() === '4278255360');
assert(h.toString() === '0,255,0,255');

// Comparator argument should be callable.
var i = Uint8Array.from([1, 2, 3]);
try {
  i.sort({});
  assert(false);
} catch (e) {
  assert(e instanceof TypeError);
}

// Comparator function returns a Number.
i.sort(function (lhs, rhs) {
  return rhs - lhs;
});
assert(i.toString() === '3,2,1');

// Comparator function returns a non-Number type that coerces to a Number.
i.sort(function (lhs, rhs) {
  return { valueOf: function() { return rhs - lhs; } };
});
assert(i.toString() === '3,2,1');
