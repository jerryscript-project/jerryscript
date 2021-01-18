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

import { getName, getNamePromise } from "./module-resource-name-export.mjs"

assert(getName().endsWith("module-resource-name-export.mjs"));

var collector = {};
getNamePromise(collector).then(() => { collector["end"] = resourceName(); });

function __checkAsync() {
  assert(collector["start"].endsWith("module-resource-name-export.mjs"));
  assert(collector["middle"].endsWith("module-resource-name-export.mjs"));
  assert(collector["end"].endsWith("module-resource-name.mjs"));
  assert(Object.keys(collector).length === 3);
}
