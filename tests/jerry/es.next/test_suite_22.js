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

(function tc_22_02_02__004() {
  function foo(v, k)
  {
    return this.num + v + k;
  }

  var a = Float32Array.from([10,20,30], foo, {num:0.5});

  assert(a[0] === 10.5);
  assert(a[1] === 21.5);
  assert(a[2] === 32.5);
})();

/* ES11 22.2.1.1 - length is equal to 0 */
(function tc_22_02_02__002() {
  var a = Object.getPrototypeOf(Int8Array);
  assert(a.length === 0);
})();

(function tc_22_02_02__003() {
  var a = Object.getPrototypeOf(Int8Array);
  var b = new Int8Array();
  var c = Object.getPrototypeOf(Object.getPrototypeOf(b));
  assert(c === a.prototype);
})();

(function tc_22_02_02__005() {
  var name = "";

  try
  {
    Int16Array.from.call(1);
  }
  catch (e)
  {
    name = e.name;
  }

  assert(name === "TypeError");

  name = "";

  try
  {
    Int16Array.from.call(Float32Array);
  }
  catch (e)
  {
    name = e.name;
  }

  assert(name === "TypeError");

  name = "";

  try
  {
    Int16Array.from.call(Float32Array, [1,2,3], 1);
  }
  catch (e)
  {
    name = e.name;
  }

  assert(name === "TypeError");

  name = "";

  try
  {
    Int16Array.from.call(Number, [1,2,3]);
  }
  catch (e)
  {
    name = e.name;
  }

  assert(name === "TypeError");
})();

(function tc_22_02_02__001() {
  var a = Object.getPrototypeOf(Int8Array);
  assert(a.name === "TypedArray");
})();

(function tc_22_02_01__013() {
  var a = new Float32Array(2);

  a[0] = 0.1;
  a[1] = -2.3;

  assert(a[0] === 0.10000000149011612);
  assert(a[1] === -2.299999952316284);
})();

(function tc_22_02_01__021() {
  var a = new Float32Array([0.1, 0.2, 0.3]);

  var b = a.hasOwnProperty(1);
  var c = a.hasOwnProperty(3);

  assert (b === true);
  assert (c === false);
})();

(function tc_22_02_01__002() {
  var a = new Int8Array(5);
  assert(a instanceof Int8Array);
})();

(function tc_22_02_01__006() {
  var a = new Int8Array([1,2,3]);
  assert(a instanceof Int8Array);
})();

(function tc_22_02_01__009() {
  Int8Array.prototype[10] = 10;
  var a = new Int8Array(5);
  assert(a[10] === undefined);
})();

(function tc_22_02_01__015() {
  var a = new Int32Array(8);

  a[0] = 0xffffffff;
  a[1] = 0xff00000001;
  a[2] = 0xff80000001;
  a[3] = -2.3;
  a[4] = Number.NEGATIVE_INFINITY;
  a[5] = NaN;
  a[6] = 10e17;
  a[7] = -10e17;

  assert(a[0] === -1);
  assert(a[1] === 1);
  assert(a[2] === -2147483647);
  assert(a[3] === -2);
  assert(a[4] === 0);
  assert(a[5] === 0);
  assert(a[6] === -1486618624);
  assert(a[7] === 1486618624);
})();

(function tc_22_02_01__018() {
  var a = new Uint16Array(2);

  a[0] = 0x123456789A;
  a[1] = -2.3;

  assert(a[0] === 0x789A);
  assert(a[1] === 65534);
})();

(function tc_22_02_01__020() {
  var name = "";

  try
  {
    new Int16Array(Float32Array.prototype);
  }
  catch (e)
  {
    name = e.name;
  }

  assert(name === "TypeError");
})();

(function tc_22_02_01__016() {
  var a = new Int8Array(3);

  a[0] = 0xff;
  a[1] = 0xff01;
  a[2] = -2.3;

  assert(a[0] === -1);
  assert(a[1] === 1);
  assert(a[2] === -2);
})();

(function tc_22_02_01__010() {
  var a = new Float32Array([0.1, -3.4, 65535.9]);
  var b = new Int16Array(a);
  var c = new Uint8Array(a);
  var d = new Int32Array(a);

  assert(b[0] === 0);
  assert(b[1] === -3);
  assert(b[2] === -1);
  assert(c[0] === 0);
  assert(c[1] === 253);
  assert(c[2] === 255);
  assert(d[0] === 0);
  assert(d[1] === -3);
  assert(d[2] === 65535);
})();

(function tc_22_02_01__008() {
  var a = new Int8Array([1.5,1000,-9]);
  a[2] = a[1] * a[0];
  assert(a[2] === -24);
})();

(function tc_22_02_01__014() {
  var a = new Int16Array(3);

  a[0] = 0xffff;
  a[1] = 0xff0001;
  a[2] = -2.3;

  assert(a[0] === -1);
  assert(a[1] === 1);
  assert(a[2] === -2);
})();

(function tc_22_02_01__007() {
  var a = new Int8Array(5);
  assert(a[2] === 0);
})();

(function tc_22_02_01__017() {
  var a = new Uint32Array(2);

  a[0] = 0x123456789A;
  a[1] = -2.3;

  assert(a[0] === 0x3456789A);
  assert(a[1] === 4294967294);
})();

(function tc_22_02_01__001() {
  var a = new Int8Array();
  assert(a instanceof Int8Array);
})();

(function tc_22_02_01__003() {
  var a = new Int8Array('5');
  assert(a instanceof Int8Array);
})();

(function tc_22_02_01__019() {
  var a = new Uint8Array(2);

  a[0] = 0x123456789A;
  a[1] = -2.3;

  assert(a[0] === 0x9A);
  assert(a[1] === 254);
})();

(function tc_22_02_01__005() {
  var a = new ArrayBuffer(5);
  var b = new Int8Array(a);
  assert(b instanceof Int8Array);
})();

(function tc_22_02_01__012() {
  var a = new Float64Array(2);

  a[0] = 0.1;
  a[1] = -2.3;

  assert(a[0] === 0.1);
  assert(a[1] === -2.3);
})();

(function tc_22_02_01__011() {
  var a = new Uint8ClampedArray([1.5, 2.5, -1.5, 10000]);

  assert(a[0] === 2);
  assert(a[1] === 2);
  assert(a[2] === 0);
  assert(a[3] === 255);
})();

(function tc_22_02_01__004() {
  var a = new Int8Array(5);
  var b = new Int8Array(a);
  assert(a instanceof Int8Array);
})();

(function tc_22_02_03__015() {
  var total = new Float32Array([-1.5, 0, 1.5, 2]).reduce(function(a, b, c) {
    return a + b + c;
  }, 10);

  assert(total === 18);
})();

(function tc_22_02_03__003() {
  var a = new Int8Array(5);
  assert(a.byteLength === 5);
})();

(function tc_22_02_03__020() {
  var uint8 = new Uint8Array(4);

  uint8.set([10, "11", 12]);
  assert(uint8[0] === 10 && uint8[1] === 11 && uint8[2] === 12);

  uint8.set([13, 14.3, 15], 1);
  assert(uint8[0] === 10 && uint8[1] === 13 && uint8[2] === 14 && uint8[3] === 15);

  uint8.set([16], NaN);
  assert(uint8[0] === 16 && uint8[1] === 13 && uint8[2] === 14 && uint8[3] === 15);

  uint8.set([17], "");
  assert(uint8[0] === 17 && uint8[1] === 13 && uint8[2] === 14 && uint8[3] === 15);

  uint8.set([18], "0");
  assert(uint8[0] === 18 && uint8[1] === 13 && uint8[2] === 14 && uint8[3] === 15);

  uint8.set([19], false);
  assert(uint8[0] === 19 && uint8[1] === 13 && uint8[2] === 14 && uint8[3] === 15);

  uint8.set([20], 0.2);
  assert(uint8[0] === 20 && uint8[1] === 13 && uint8[2] === 14 && uint8[3] === 15);

  uint8.set([21], 0.9);
  assert(uint8[0] === 21 && uint8[1] === 13 && uint8[2] === 14 && uint8[3] === 15);

  uint8.set([22], null);
  assert(uint8[0] === 22 && uint8[1] === 13 && uint8[2] === 14 && uint8[3] === 15);

  uint8.set([23], {});
  assert(uint8[0] === 23 && uint8[1] === 13 && uint8[2] === 14 && uint8[3] === 15);

  uint8.set([24], []);
  assert(uint8[0] === 24 && uint8[1] === 13 && uint8[2] === 14 && uint8[3] === 15);

  uint8.set([25], true);
  assert(uint8[0] === 24 && uint8[1] === 25 && uint8[2] === 14 && uint8[3] === 15);
})();

(function tc_22_02_03__011() {
  var a = new Float32Array([1.25, 2.5, 3.75]);

  var b = a.map(function(num) {
    return num * 2;
  });

  assert(a[0] === 1.25);
  assert(a[1] === 2.5);
  assert(a[2] === 3.75);
  assert(b[0] === 2.5);
  assert(b[1] === 5);
  assert(b[2] === 7.5);
})();

(function tc_22_02_03__002() {
  var a = new Int8Array(5);
  assert(a.byteOffset === 0);
})();

(function tc_22_02_03__012() {
  var a = new Float32Array([1.1, 2.2, 3.3, 4.4]);
  var count = 0;

  function f_every(num)
  {
    count++;
    return num < 3;
  }

  var ret = a.every(f_every);

  assert(ret === false);
  assert(count === 3);
})();

(function tc_22_02_03__007() {
  var a = new Uint8Array([10, 20, 30, 40]);
  var o = {
    "small":0,
    "large":0
  };
  var func = function(v, k)
  {
    if (v < 25)
    {
      this.small = this.small + k;
    }
    else
    {
      this.large = this.large + k;
    }
  }

  var ret = a.forEach(func, o);

  assert(ret === undefined);
  assert(o.small === 1);
  assert(o.large === 5);
})();

(function tc_22_02_03__021() {
  var float64 = new Float64Array(4);
  float64.set([10.1, "11.2", 12.3]);
  assert(float64[0] === 10.1 && float64[1] === 11.2 && float64[2] === 12.3);

  float64.set([13.1, 14.2, 15.3], 1);
  assert(float64[0] === 10.1 && float64[1] === 13.1 && float64[2] === 14.2 && float64[3] === 15.3);

  try
  {

    float64.set([17.1, 18.2, 19.3], 2);
    assert(false);
  } catch (e)
  {
    assert(e instanceof RangeError)
  }
})();

(function tc_22_02_03__009() {
  var a = new Uint8Array([1, 2, 3, 4]);
  var count = 0;

  function f_every(num)
  {
    count++;
    return num < 3;
  }

  var ret = a.every(f_every);

  assert(ret === false);
  assert(count === 3);
})();

(function tc_22_02_03__014() {
  var a = new Float32Array([-1.1, 0.1, 2.5, 3.0]);
  var o = {
    "small":0,
    "large":0
  };
  var func = function(v, k)
  {
    if (v < 2)
    {
      this.small = this.small + k;
    }
    else
    {
      this.large = this.large + k;
    }
  }

  var ret = a.forEach(func, o);

  assert(ret === undefined);
  assert(o.small === 1);
  assert(o.large === 5);
})();

(function tc_22_02_03__001() {
  var a = new Int8Array(5);
  assert(a.length === 5);
})();

(function tc_22_02_03__010() {
  var a = new Uint8Array([1, 2, 3, 4]);
  var count = 0;

  function f_some(num)
  {
    count++;
    return num > 3;
  }

  var ret = a.some(f_some);

  assert(ret === true);
  assert(count === 4);
})();

(function tc_22_02_03__006() {
  var a = new Int8Array([1,2,3,4,5]);
  var b = new Int8Array(a.buffer, 2, 3);

  b[0] = 5.6;
  assert(a[2] === 5);
})();

(function tc_22_02_03__018() {
  var a = new Float32Array([-1.5, 0, 1.5]);
  var b = a.reverse()

  assert(a === b);
  assert(a[0] === 1.5);
  assert(a[1] === 0);
  assert(a[2] === -1.5);
})();

(function tc_22_02_03__008() {
  var a = new Uint8Array([1, 2, 3]);

  var b = a.map(function(num) {
    return num * 2;
  });

  assert(a[0] === 1);
  assert(a[1] === 2);
  assert(a[2] === 3);
  assert(b[0] === 2);
  assert(b[1] === 4);
  assert(b[2] === 6);
})();

(function tc_22_02_03__016() {
  var total = new Float32Array([-1.5, 0, 1.5, 2]).reduceRight(function(a, b) {
    return a - b;
  });

  assert (total === 2)
})();

(function tc_22_02_03__013() {
  var a = new Float32Array([1.1, 2.2, 3.3, 4.4]);
  var count = 0;

  function f_some(num)
  {
    count++;
    return num > 3;
  }

  var ret = a.some(f_some);

  assert(ret === true);
  assert(count === 3);
})();

(function tc_22_02_03__005() {
  var a = new Int8Array([1,2,3,4,5]);
  var b = new Int8Array(a.buffer, 2, 3);

  assert(a.buffer === b.buffer);
  assert(b.length === 3);
  assert(b.byteOffset === 2);
  assert(b.byteLength === 3);
})();

(function tc_22_02_03__017() {
  var a = new Float32Array([-1.5, 0, 1.5, 2]);
  var b = a.filter(function(x){
    return x > 0;
  });

  assert(a[0] === -1.5);
  assert(a[1] === 0);
  assert(a[2] === 1.5);
  assert(a[3] === 2);
  assert(b[0] === 1.5);
  assert(b[1] === 2);
  assert(b.length === 2);
})();

(function tc_22_02_03__019() {
  var uint8 = new Uint8Array(4);


  assert(uint8.set.length === 1)

  try
  {
    uint8.set([1], -1);
    assert(false);
  } catch (e)
  {
    assert(e instanceof RangeError);
  }

  try
  {
    uint8.set([1], - (Math.pow(2, 32) + 1));
    assert(false);
  } catch (e)
  {
    assert(e instanceof RangeError);
  }

  try
  {
    uint8.set([1], -Infinity);
    assert(false);
  } catch (e)
  {
    assert(e instanceof RangeError);
  }

  try
  {
    uint8.set([1], Infinity);
    assert(false);
  } catch (e)
  {
    assert(e instanceof RangeError);
  }

  try
  {
    uint8.set([1], (Math.pow(2, 32) + 1));
    assert(false);
  } catch (e)
  {
    assert(e instanceof RangeError);
  }

  try
  {

    uint8.set([17, 18, 19], 2);
    assert(false);
  } catch (e)
  {
    assert(e instanceof RangeError)
  }
})();

(function tc_22_02_03__004() {
  var a = new Int8Array(5);
  assert(a.buffer instanceof ArrayBuffer);
})();

(function tc_22_02_06__001() {
  assert(Int8Array.prototype.BYTES_PER_ELEMENT === 1);
  assert(Uint8Array.prototype.BYTES_PER_ELEMENT === 1);
  assert(Uint8ClampedArray.prototype.BYTES_PER_ELEMENT === 1);
  assert(Int16Array.prototype.BYTES_PER_ELEMENT === 2);
  assert(Uint16Array.prototype.BYTES_PER_ELEMENT === 2);
  assert(Int32Array.prototype.BYTES_PER_ELEMENT === 4);
  assert(Uint32Array.prototype.BYTES_PER_ELEMENT === 4);
  assert(Float32Array.prototype.BYTES_PER_ELEMENT === 4);
  assert(Float64Array.prototype.BYTES_PER_ELEMENT === 8);
})();

(function tc_22_02_05__001() {
  assert(Int8Array.BYTES_PER_ELEMENT === 1);
  assert(Uint8Array.BYTES_PER_ELEMENT === 1);
  assert(Uint8ClampedArray.BYTES_PER_ELEMENT === 1);
  assert(Int16Array.BYTES_PER_ELEMENT === 2);
  assert(Uint16Array.BYTES_PER_ELEMENT === 2);
  assert(Int32Array.BYTES_PER_ELEMENT === 4);
  assert(Uint32Array.BYTES_PER_ELEMENT === 4);
  assert(Float32Array.BYTES_PER_ELEMENT === 4);
  assert(Float64Array.BYTES_PER_ELEMENT === 8);
})();
