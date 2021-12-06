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

var realm = createRealm()

function compare(a, b)
{
  assert (a === b)
  assert (typeof a === "symbol")
  assert (typeof b === "symbol")
}

compare(Symbol.asyncIterator, realm.Symbol.asyncIterator)
compare(Symbol.hasInstance, realm.Symbol.hasInstance)
compare(Symbol.isConcatSpreadable, realm.Symbol.isConcatSpreadable)
compare(Symbol.iterator, realm.Symbol.iterator)
compare(Symbol.match, realm.Symbol.match)
compare(Symbol.replace, realm.Symbol.replace)
compare(Symbol.search, realm.Symbol.search)
compare(Symbol.species, realm.Symbol.species)
compare(Symbol.split, realm.Symbol.split)
compare(Symbol.toPrimitive, realm.Symbol.toPrimitive)
compare(Symbol.toStringTag, realm.Symbol.toStringTag)
compare(Symbol.unscopables, realm.Symbol.unscopables)
