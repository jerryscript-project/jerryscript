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

var a;
Promise.race([a]).then(function() {[] = []});
Promise.race().then(function() {}, function() { throw "'this' had incorrect value!"})

var a = Promise.resolve('a');
var b = Promise.reject('b');
Promise.race([a, b]).then(function(x) {
    var [a, b] = [1, 2];
}, function(x) {});
Promise.race([b, a]).then(function(x) {}, function(x) {});
Promise.race([, b, a]).then(function(x) {}, function(x) {});
Promise.race(a).then(function(x) {}, function(x) {
    String(i.name === "TypeError");
});
