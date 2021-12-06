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

/* Test the Date.prototype.toJSON method's internal operation order
 * by creating a Proxy to catch the method invokations.
 */
var found = [];

var target = {
  toString: function() { return "TARGET_toString"; },
  toISOString: function() { return "TARGET_toISOString"; }
};

var prox = new Proxy(target, {
  get: function(trg, key) {
    found.push(key);
    return trg[key];
  }
});

/* Date.prototype.toJSON -> ... -> <target>.toISOString() */
var json_result = Date.prototype.toJSON.call(prox);
assert(json_result === "TARGET_toISOString");

/* Data.prototype.toJSON -> toPrimitive -> Get -> [[Get]]
 * The first element in the "found" properties should be the "@@toPrimitive" well-known Symbol.
 */
assert(found[0] === Symbol.toPrimitive);

/* Date.prototype.toJSON -> Invoke -> GetMethod -> GetV -> [[Get]]
 * All other elements are "methods" and there should be no duplicates.
 */
var methods = found.slice(1);
assert(methods.toString() === "valueOf,toString,toISOString");
