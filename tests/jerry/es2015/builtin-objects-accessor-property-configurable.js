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

var test_failed = false;

function verifyConfigurableAccessor (obj, name, string) {
  let prop = Object.getOwnPropertyDescriptor (obj, name);
  if (prop.get && !prop.configurable) {
    print (string + " should be configurable, but wasn't");
    test_failed = true;
  }
}

for (let builtin_name of Reflect.ownKeys (this)) {
  let builtin_obj = this[builtin_name];
  if (builtin_name[0] === builtin_name[0].toUpperCase () && typeof builtin_obj == "function") {
    for (let prop of Reflect.ownKeys (builtin_obj)) {
      verifyConfigurableAccessor (builtin_obj, prop, builtin_name + "." + prop.toString ());
    }

    let builtin_proto = builtin_obj.prototype;
    if (builtin_proto) {
      for (let prop of Reflect.ownKeys (builtin_proto)) {
        verifyConfigurableAccessor (builtin_proto, prop, builtin_name + ".prototype." + prop.toString ());
      }
    }

    builtin_proto = Reflect.getPrototypeOf (builtin_obj);
    if (builtin_proto !== Function.prototype) {
      for (let prop of Reflect.ownKeys (builtin_proto.prototype)) {
        verifyConfigurableAccessor (builtin_proto.prototype, prop, builtin_name + ".[[Prototype]].prototype." + prop.toString ());
      }
    }
  }
}

assert (!test_failed);
