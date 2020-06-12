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

// Original issue
Object.defineProperty(Object.prototype, 6, {});
Promise.all([0]);

// Variant 2
Object.defineProperty(Object.prototype, 2, {});
Promise.all([0]);

// Variant 3
Object.defineProperty(Object.prototype, 3, {});
Promise.all([0]);

// Variant 4
Object.defineProperty(Object.prototype, 4, {});
Promise.all([0]);

// Variant 5
Object.defineProperty(Object.prototype, 5, {});
Promise.all([0]);

// Variant 7
Object.defineProperty(Object.prototype, 7, {});
Promise.all([0]);

// Variant 8
Object.defineProperty(Object.prototype, 8, {});
Promise.all([0]);
