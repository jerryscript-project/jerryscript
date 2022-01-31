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

var target_obj = {};
var not_target_obj = {};
var weak_ref = new WeakRef(target_obj);
var weak_ref2 = new WeakRef(target_obj);

assert (weak_ref.deref() === target_obj);
assert (weak_ref.deref() === weak_ref2.deref());
assert (weak_ref.deref() !== not_target_obj);

assert (weak_ref2.deref() === target_obj);
assert (weak_ref2.deref() !== not_target_obj);
assert (weak_ref2.deref() === weak_ref2.deref());

target_obj = undefined;
gc();

assert (weak_ref.deref() === undefined);
assert (weak_ref2.deref() === undefined);

var key_obj_1 = {};
var key_obj_2 = {};
var key_obj_3 = {};
var target_obj_2 = {};
var target_obj_3 = {};
var weak_ref_3 = new WeakRef(key_obj_1);
var weak_ref_4 = new WeakRef(target_obj_2);
var weak_ref_5 = new WeakRef(target_obj_3);

var weak_map = new WeakMap();
weak_map.set(key_obj_1, weak_ref_3);
weak_map.set(key_obj_2, weak_ref_4);
weak_map.set(key_obj_3, weak_ref_5);

assert(weak_map.has(key_obj_1));
assert(weak_map.has(key_obj_2));
assert(weak_map.has(key_obj_3));

assert(weak_map.get(key_obj_1).deref() === key_obj_1);
assert(weak_map.get(key_obj_2).deref() === target_obj_2);
assert(weak_map.get(key_obj_3).deref() === target_obj_3);

key_obj_1 = undefined;
gc();
assert(weak_map.get(key_obj_1) === undefined);
assert(weak_ref_3.deref() === undefined);

key_obj_2 = undefined;
gc();
assert(weak_map.get(key_obj_2) === undefined);
assert(weak_ref_4.deref() === target_obj_2);

target_obj_3 = undefined;
gc();
assert(weak_map.get(key_obj_3) !== undefined);
assert(weak_ref_5.deref() === undefined);
