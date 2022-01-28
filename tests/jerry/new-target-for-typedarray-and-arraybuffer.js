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

class MyInt32Array extends Int32Array {};
var t1= new MyInt32Array([1,2]);

assert(Object.getPrototypeOf(t1) == MyInt32Array.prototype)


class MyArrayBuffer extends ArrayBuffer {};
var t2= new MyArrayBuffer(8);

assert(Object.getPrototypeOf(t2) == MyArrayBuffer.prototype)
