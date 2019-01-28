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

var recursion_counter = 0;
var recursion_limit = 1000;
var fz_globalObject = this

function fz_is_primitive(value) {
    var value_type = typeof value
    if (value_type !== "function" && value_type !== "object")
        return value_type
}

function fz_starts_with(str, pattern) {
    for (var i = 0; i < pattern.length; i++)
        if (str[i] !== pattern[i])
            return
    return true
}

function fz_collect_values(value) {
    if (++recursion_counter >= recursion_limit) {
      return
    }

    var primitive = fz_is_primitive(value)
    if (primitive)
        return
    var prop_names = Object.getOwnPropertyNames(value)
    for (var i = 0; i < prop_names.length; i++) {
        var prop_name = prop_names[i]
        if (!fz_starts_with(prop_name, "fz_")) {
            fz_collect_values(value[prop_name])
        }
    }
}

fz_collect_values(fz_globalObject)
