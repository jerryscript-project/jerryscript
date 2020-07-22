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

var o = {};
o.__defineGetter__('foo', function() { return 5; });
assert(o.foo === 5);

var obj = {};
function bar() { return "bar"; }
Object.prototype.__defineGetter__.call(obj, "foo", bar);
var prop = Object.getOwnPropertyDescriptor(obj, "foo");
assert(prop.get == bar);
assert(!prop.writable);
assert(prop.configurable);
assert(prop.enumerable);

var obj = {};
var sym = Symbol();
function bar() { return "bar"; }
Object.prototype.__defineGetter__.call(obj, sym, bar);
var prop = Object.getOwnPropertyDescriptor(obj, sym);
assert(prop.get == bar);
assert(!prop.writable);
assert(prop.configurable);
assert(prop.enumerable);;

var key = '__accessors_test__';
Object.prototype.__defineGetter__.call(1, key, function(){});

try {
  Object.prototype.__defineGetter__.call(null, key, function(){});
  assert(false);
} catch(e){
  assert(e instanceof TypeError);
}

var def = [];
var p = new Proxy({}, { defineProperty: function(o, v, desc) { def.push(v); Object.defineProperty(o, v, desc); return true; }});
Object.prototype.__defineGetter__.call(p, "foo", Object);
assert(def + '' === "foo");
