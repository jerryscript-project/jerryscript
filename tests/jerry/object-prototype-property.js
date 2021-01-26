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

// Tests that if multiple properties with the same name is
// in the prototype chain, only the first one is inspected

var not_called = 0;
var called = 0;

function accessor_proto() {
  Object.defineProperty(this, "prop",
    { get: function() { return 3.5 }, set: function(v) { called++ } }
  )
}

function data_proto() {
  Object.defineProperty(this, "prop",
    { value:7, writable:true }
  )
}

accessor_proto.prototype = { get prop() { not_called++ }, set prop(v) { not_called++ } }
data_proto.prototype = accessor_proto.prototype

function create_accessor() {}
function create_data() {}

create_accessor.prototype = new accessor_proto();
create_data.prototype = new data_proto();

var o = new create_accessor()
o.prop = 1
assert(o.prop === 3.5)
assert(Object.getPrototypeOf(o) === create_accessor.prototype)

o = new create_data()
o.prop = 'X'
assert(o.prop === 'X')
assert(Object.getPrototypeOf(o) === create_data.prototype)

assert(not_called === 0)
assert(called === 1)
