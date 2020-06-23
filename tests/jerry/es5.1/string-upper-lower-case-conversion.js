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

// Although codepoint 0x10400 and 0x10428 are an upper-lowercase pair,
// we must not do their conversion in JavaScript. We must also ignore
// stray surrogates.

assert ("\ud801\ud801\udc00\udc00".toLowerCase() == "\ud801\ud801\udc00\udc00");
assert ("\ud801\ud801\udc28\udc28".toUpperCase() == "\ud801\ud801\udc28\udc28");
