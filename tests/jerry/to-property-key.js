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

/* Test if the ToPropertyKey operation calls the @@ToPrimitive symbol */

var obj_A = {};
var sym = Symbol();

var toprimitive_called = 0;

var wrap = {
    [Symbol.toPrimitive]: function() {
        toprimitive_called++;
        return sym;
    }
};

/* <object>.hasOwnProperty calls the ToPropertyKey operation */
assert (obj_A.hasOwnProperty (wrap) === false);
assert (toprimitive_called === 1);

/* "in" operator calls the ToPropertyKey operation */
assert ((wrap in obj_A) === false);
assert (toprimitive_called === 2);

obj_A[sym] = 0;

/* <object>.hasOwnProperty calls the ToPropertyKey operation */
assert (obj_A.hasOwnProperty (wrap) === true);
assert (toprimitive_called === 3);

/* "in" operator calls the ToPropertyKey operation */
assert ((wrap in obj_A) === true);
assert (toprimitive_called === 4);


var obj_B = {};

/* <object>.hasOwnProperty calls the ToPropertyKey operation */
assert (obj_B.hasOwnProperty (wrap) === false);
assert (toprimitive_called === 5);

/* Object.defineProperty calls the ToPropertyKey operation */
Object.defineProperty (obj_B, wrap, {
  value: -1,
  enumerable: false,
  configurable: true,
  writable: true
});
assert (toprimitive_called === 6);

assert (obj_B[sym] === -1);

/* Object.getOwnPropertyDescriptor calls the ToPropertyKey operation */
var desc = Object.getOwnPropertyDescriptor (obj_B, wrap);
assert (toprimitive_called === 7);

assert (desc.value === -1);
assert (desc.enumerable === false);
assert (desc.configurable === true);
assert (desc.writable === true);

/* Reflect.get calls the ToPropertyKey operation */
assert (Reflect.get (obj_B, wrap) === -1);
assert (toprimitive_called === 8);

/* Reflect.deleteProperty calls the ToPropertyKey operation */
assert (Reflect.deleteProperty (obj_B, wrap) === true);
assert (toprimitive_called === 9);

/* Reflect.has calls the ToPropertyKey operation */
assert (Reflect.has (obj_B, wrap) === false);
assert (toprimitive_called === 10);

/* Reflect.defineProperty calls the ToPropertyKey operation */
Reflect.defineProperty (obj_B, wrap, {
  value: 50,
  enumerable: false,
  configurable: true,
  writable: true
});

assert (toprimitive_called === 11);

/* Reflect.getOwnPropertyDescriptor calls the ToPropertyKey operation */
var desc_B = Reflect.getOwnPropertyDescriptor (obj_B, wrap);
assert (toprimitive_called === 12);

assert (desc_B.value === 50);
assert (desc_B.enumerable === false);
assert (desc_B.configurable === true);
assert (desc_B.writable === true);

/* Delete calls the ToPropertyKey operation */
delete obj_B[wrap];
assert (toprimitive_called === 13);

/* Get calls the ToPropertyKey operation */
assert (obj_B[wrap] === undefined);
assert (toprimitive_called === 14);

/* Computed property calls the ToPropertyKey operation */
var obj_C = {
    [wrap]: function() { return 100; }
};
assert (toprimitive_called === 15);
assert (obj_C[sym]() === 100);

/* Setter/Getter will each call the ToPropertyKey operation */
var obj_D = {
    get [wrap] () { return 150; },
    set [wrap] (value) { }
};
assert (toprimitive_called === 17);
assert (obj_D[sym] === 150);
