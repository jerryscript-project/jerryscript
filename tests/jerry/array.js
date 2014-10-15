// Copyright 2014 Samsung Electronics Co., Ltd.
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

var cars = ["Saab", "Volvo", "BMW"];

assert (cars[0] === "Saab");
assert (cars[1] === "Volvo");
assert (cars[2] === "BMW");

var cars1 = new Array("Saab", "Volvo", "BMW");
assert (cars[0] === cars1[0]);
assert (cars[1] === cars1[1]);
assert (cars[2] === cars1[2]);

var a = new Array();
assert (typeof (a) === "object");
assert (Array.isArray (a));
assert (Array.isArray ([1, 2, 3]));

var b = new Array (30000);
assert(b.length === 30000);
assert (b[20000] === undefined);
b[20000] = 1;
assert (b[20000] === 1);
b[20000] = 10;
assert (b[20000] === 10);

assert(b.length === 30000);
assert(b[10000] === undefined);
Object.defineProperty (b, '10000', {value : 25, writable : false});
assert(b[10000] === 25);
b[10000] = 30;
assert(b[10000] === 25);

assert(b.length === 30000);
assert(b[50000] === undefined);
assert(b.length === 30000);
b[50000] = 5;
assert(b.length === 50001);
assert(b[50000] === 5);
b[50000] = 10;
assert(b[50000] === 10);
Object.defineProperty (b, '50000', {writable : false});
assert(b[50000] === 10);
b[50000] = 20;
assert(b[50000] === 10);

Object.defineProperty (b, '50000', {writable : true});
assert(b[50000] === 10);
b[50000] = 30;
assert(b[50000] === 30);

b.length = 5;
assert(b[50000] === undefined);

assert(([1, 2, 3]).length === 3);

assert(Array.prototype.constructor === Array);
assert(Array.prototype.length === 0);
Array.prototype[0] = 'string value';
assert(Array.prototype.length === 1);
assert(Array.prototype[0] === 'string value');

var c = [0,,,'3'];
assert (c[0] === 0);
assert (c[1] === undefined);
assert (c[2] === undefined);
assert (c[3] === '3');
