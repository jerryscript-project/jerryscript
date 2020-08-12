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

/* Test JSON parse [[delete]] with Array */
var badDeleteArray = new Proxy([0], {
  deleteProperty: function() {
    throw "My Super Error A";
  }
});

try {
  JSON.parse('[0,0]', function() { this[1] = badDeleteArray; });
} catch (ex) {
  assert(ex === "My Super Error A");
}

/* Test JSON parse [[delete]] with Objects */
var badDeleteObj = new Proxy([0], {
  deleteProperty: function() {
    throw "My Super Error B";
  }
});

try {
  JSON.parse('[0,0]', function() { this[1] = badDeleteObj; });
} catch (ex) {
  assert(ex === "My Super Error B");
}

/* Test JSON parse property define with Array */
var badDefineArray = new Proxy([null], {
  defineProperty: function(_, name) {
    throw "My Super Error C";
  }
});

try {
  JSON.parse('["first", null]', function(_, value) {
    if (value === 'first') {
      this[1] = badDefineArray;
    }
    return value;
  });
} catch (ex) {
  assert(ex === "My Super Error C");
}

/* Test JSON parse property define with Object */
var badDefineObj = new Proxy({0: null}, {
  defineProperty: function(_, name) {
    throw "My Super Error D";
  }
});

try {
  JSON.parse('["first", null]', function(_, value) {
    if (value === 'first') {
      this[1] = badDefineObj;
    }
    return value;
  });
} catch (ex) {
  assert(ex === "My Super Error D");
}

/* Test JSON parse keys call */
var badKeys = new Proxy({}, {
  ownKeys: function() {
    throw "My Super Error E";
  }
});

try {
  JSON.parse('[0,0]', function() { this[1] = badKeys; });
} catch (ex) {
  assert(ex === "My Super Error E");
}
