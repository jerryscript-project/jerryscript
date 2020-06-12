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

// Constructor creates longer array than expected.
class LongArray extends Array {
    constructor(len) {
        super (42);
        this.fill ("foo");
    }
}

var a = new LongArray (5);
a.length = 5;
var sliced = a.slice ();
assert (sliced.length == 5);
assert (JSON.stringify (sliced) == '["foo","foo","foo","foo","foo"]')

// Constructor creates shorter array than expected.
class ShortArray extends Array {
    constructor(len) {
        super (2);
        this.fill ("bar");
    }
}

var b = new ShortArray (8);
b.length = 8;
b.fill ("asd", 2);
var sliced2 = b.slice ();
assert (sliced2.length == 8);
assert (JSON.stringify (sliced2) == '["bar","bar","asd","asd","asd","asd","asd","asd"]');

// Constructor creates array of the expected size.
class ExactArray extends Array {
    constructor(len) {
        super (len);
        this.fill ("baz");
    }
}

var c = new ExactArray (5);
var sliced3 = c.slice();
assert (sliced3.length == 5);
assert (JSON.stringify (sliced3) == '["baz","baz","baz","baz","baz"]');
