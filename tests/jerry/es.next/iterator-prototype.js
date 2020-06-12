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

var array = [];
var array_iterator = array[Symbol.iterator]();
var array_iterator_prototype = Object.getPrototypeOf (array_iterator);
var iterator_prototype = Object.getPrototypeOf (array_iterator_prototype);
var iterator_prototype_iterator = iterator_prototype[Symbol.iterator]();

assert (iterator_prototype === iterator_prototype_iterator);
