// Copyright 2015 Samsung Electronics Co., Ltd.
// Copyright 2015 University of Szeged.
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

// Test raised by fuzzer
v_0 = [,];
v_1 = [,];
v_2 = Object.defineProperties([,], { '0': {  get: function() { } } });

// Test change from data to accessor type
var a = { x:2 };
Object.defineProperty(a, "x", {
      enumerable: true,
      configurable: true,
      get: function() { return 0; }
});

// Test change from accessor to data type
var obj = {test: 2};

Object.defineProperty(obj, "test", {
      enumerable: true,
      configurable: true,
      get: function() { return 0; }
});

Object.defineProperty(obj, "x", {
      enumerable: true,
      configurable: true,
      value: -2
});
