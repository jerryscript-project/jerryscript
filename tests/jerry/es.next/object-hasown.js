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

var obj = { "foo": 5 };

assert(Object.hasOwn(obj, "foo") == true);
assert(Object.hasOwn(obj, "bar") == false);

const handler1 = {};

const handler2 = {
    getOwnPropertyDescriptor(target, prop, receiver) {
        return { configurable: true, enumerable: true, value: true };
    }
};

const handler3 = {
    getOwnPropertyDescriptor(target, prop, receiver) { }
};

const proxy1 = new Proxy(obj, handler1);
const proxy2 = new Proxy(obj, handler2);
const proxy3 = new Proxy(obj, handler3);

assert(Object.hasOwn(proxy1, "foo") == true);
assert(Object.hasOwn(proxy1, "bar") == false);

assert(Object.hasOwn(proxy2, "foo") == true);
assert(Object.hasOwn(proxy2, "bar") == true);

assert(Object.hasOwn(proxy3, "foo") == false);
assert(Object.hasOwn(proxy3, "bar") == false);

try {
    Object.hasOwn({}, { get toString() { throw "foo" } });
} catch (e) {
    assert(e === "foo");
}

try {
    Object.hasOwn(undefined);
    assert(false);
} catch (e) {
    assert(e instanceof TypeError);
}
