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

Number.isInteger = Number.isInteger || function (value) {
  return typeof value === 'number' &&
    isFinite (value) &&
    Math.floor (value) === value;
};

var now = Date.now ();

assert (Number.isInteger (now));

var date = new Date(now); // Should not throw an error
