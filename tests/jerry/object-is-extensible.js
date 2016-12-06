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

// New objects are extensible.
var empty = {};
assert (Object.isExtensible(empty) === true);

// ...but that can be changed.
Object.preventExtensions(empty);
assert(Object.isExtensible(empty) === false);

// Call on undefined should throw TypeError.
try
{
    Object.isExtensible(undefined);
    assert (false);
} catch (e) {
    assert (e instanceof TypeError);
}

try
{
    Object.preventExtensions(undefined);
    assert (false);
} catch (e) {
    assert (e instanceof TypeError);
}

// Sealed objects are by definition non-extensible.
var sealed = Object.seal({});
assert (Object.isExtensible(sealed) === false);

// Frozen objects are also by definition non-extensible.
var frozen = Object.freeze({});
assert(Object.isExtensible(frozen) === false);
