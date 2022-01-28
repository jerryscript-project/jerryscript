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

var a = [1, 2, 3, 4];
var b = a.slice(0, { valueOf: function(){ a.length = 0; return 100; } });

assert(b.length === 4);

var c = [1, 2, 3, 4];
c.prop = 4
var d = c.slice(0, { valueOf: function(){ c.length = 0; return 100; } });

assert(d.length === 4);
