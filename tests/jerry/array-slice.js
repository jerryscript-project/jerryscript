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

let buf = new ArrayBuffer(10);
let a1 = new Int8Array(buf, 0, 5);
a1.fill(1);
a1.constructor = {
    [Symbol.species]: function (len) {
        return new Int8Array(buf, 5, 5);
    }
};
let a2 = a1.slice(2,4);
res = new Int8Array(buf, 0, 10);

//Expected: 1, 1, 1, 1, 1, 1, 1, 0, 0, 0
for (let i = 0; i < 10; i++) {
    if (i < 7) {
        assert(res[i] === 1);
    } else {
        assert(res[i] === 0);
    }
}
