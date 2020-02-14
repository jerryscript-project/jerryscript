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

var builtin_objects = [
  Array,
  ArrayBuffer,
  Boolean,
  DataView,
  Date,
  Error,
  EvalError,
  Function,
  Map,
  Number,
  Object,
  Promise,
  RangeError,
  ReferenceError,
  RegExp,
  Set,
  String,
  Symbol,
  SyntaxError,
  TypeError,
  URIError,
  WeakMap,
  WeakSet,
];

var builtin_prototypes = [Function.prototype]

var builtin_typedArrays = [
  Float32Array,
  Float64Array,
  Int16Array,
  Int32Array,
  Int8Array,
  Uint16Array,
  Uint32Array,
  Uint8Array,
  Uint8ClampedArray,
];


(function () {
  /* each length property is configurable */
  var desc;
  
  for (obj of builtin_objects) {
    desc = Object.getOwnPropertyDescriptor(obj, 'length');                                                                                                                    
    assert(desc.writable === false);
    assert(desc.enumerable === false);
    assert(desc.configurable === true);
  }
  
  for (proto of builtin_prototypes) {
    desc = Object.getOwnPropertyDescriptor(proto, 'length');                                                                                                                    
    assert(desc.writable === false);
    assert(desc.enumerable === false);
    assert(desc.configurable === true);
  }
  
  for (ta of builtin_typedArrays) {
    desc = Object.getOwnPropertyDescriptor(ta, 'length');                                                                                                                    
    assert(desc.writable === false);
    assert(desc.enumerable === false);
    assert(desc.configurable === true);
  }
})();

(function () {
  /* each length property can be deleted */
  for (obj of builtin_objects) {
    assert(obj.hasOwnProperty('length') === true);
    assert(delete obj.length);
    assert(obj.hasOwnProperty('length') === false);
  }

  for (proto of builtin_prototypes) {
    assert(proto.hasOwnProperty('length') === true);
    assert(delete proto.length);
    assert(proto.hasOwnProperty('length') === false);
  }

  for (ta of builtin_typedArrays) {
    assert(ta.hasOwnProperty('length') === true);
    assert(delete ta.length);
    assert(ta.hasOwnProperty('length') === false);
  }
})();
