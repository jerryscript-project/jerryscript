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

function member_str() {
  return "member";
}

switch (true) {
default:
  var obj = {
    ["val" + "ue"]: 0,
    set[member_str()](x) {
      // Multiple statements.
      this.value = x + 4;
      this.value += 2;
    },
    get[member_str() ? member_str() : ""]() {
      // Multiple statements.
      this.value = this.value + 1;
      return this.value;
    },
    get
      [1 + 2]
      ()
    {
      return 3;
    },
    [false ? member_str()
           : ""]
     :8
  }
}

obj["member"] = 10;
assert(obj.member === 17);
assert(obj.member === 18);

assert(obj[3] === 3);
assert(obj["3"] === 3);

assert(obj[""] === 8);
