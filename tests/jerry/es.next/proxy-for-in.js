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

// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var handlerBoolean = {
  ownKeys: function() { return true; },
}
var boolProxy = new Proxy({}, handlerBoolean);

try {
  for (var a in boolProxy) {
    // This should never be called.
    assert (false);
  }

  assert (false);
} catch (ex) {
  assert (ex instanceof TypeError);
}


var handlerSymbol = {
  ownKeys: function() { return Symbol("alma"); },
}
var symbolProxy = new Proxy({}, handlerSymbol);

try {
  for (var a in symbolProxy) {
    // This should never be called.
    assert (false);
  }

  assert (false);
} catch (ex) {
  assert (ex instanceof TypeError);
}


var handlerNumber = {
  ownKeys: function() { return 1; },
}
var numberProxy = new Proxy({}, handlerNumber);

try {
  for (var a in numberProxy) {
    // This should never be called.
    assert (false);
  }

  assert (false);
} catch (ex) {
  assert (ex instanceof TypeError);
}


var handlerObject = {
  ownKeys: function() { return {}; },
}
var objectProxy = new Proxy({}, handlerObject);

for (var a in objectProxy) {
  // This should never be called.
  assert (false);
}
