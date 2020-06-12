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

var search = RegExp.prototype[Symbol.search];

try {
  search.call (0, "string");
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  search.call (new RegExp(), {
    toString: () => {
      throw "abrupt string"
    }
  });
  assert (false);
} catch (e) {
  assert (e === "abrupt string");
}

try {
  search.call ({
    get lastIndex() {
      throw "abrupt get lastIndex"
    }
  }, "string");
  assert (false);
} catch (e) {
  assert (e === "abrupt get lastIndex");
}

try {
  search.call ({
    get lastIndex() {
      return 3;
    },
    set lastIndex(idx) {
      throw "abrupt set lastIndex"
    }
  }, "string");
  assert (false);
} catch (e) {
  assert (e === "abrupt set lastIndex");
}

try {
  search.call ({
    get exec() {
      throw "abrupt exec"
    }
  }, "string");
  assert (false);
} catch (e) {
  assert (e === "abrupt exec");
}

try {
  search.call ({
    exec: RegExp.prototype.exec
  }, "string");
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  search.call ({
    exec: 42
  }, "string");
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  search.call ({
    exec: () => {
      throw "abrupt exec result"
    }
  }, "string");
  assert (false);
} catch (e) {
  assert (e === "abrupt exec result");
}

try {
  search.call ({
    exec: () => {
      return 1
    }
  }, "string");
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  search.call ({
    exec: () => {
      return {
        get index() {
          throw "abrupt index"
        }
      }
    }
  }, "string");
  assert (false);
} catch (e) {
  assert (e === "abrupt index");
}

assert (search.call (/abc/, "abc") === 0);
assert (search.call (/abc/, "strabc") === 3);
assert (search.call (/abc/, "bcd") === -1);

class Regexplike {
  constructor() {
    this.index = 0;
    this.global = true;
  }

  exec() {
    if (this.index > 0) {
      return null;
    }

    this.index = 42;
    var result = {
      length: 1,
      0: "Duck",
      index: this.index
    };
    return result;
  }
}

re = new Regexplike();
assert (search.call (re, "str") === 42);

/* Object with custom @@search method */
var o = {}
o[Symbol.search] = function () {
  return 4;
};
assert ("string".search (o) === 4);

o[Symbol.search] = 42;
try {
  "string".search (o);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

Object.defineProperty (o, Symbol.search, {
  get: () => {
    throw "abrupt @@search get"
  },
  set: (v) => {}
});

try {
  "string".search (o);
  assert (false);
} catch (e) {
  assert (e === "abrupt @@search get");
}

o = {};
o[Symbol.search] = function () {
  throw "abrupt @@search"
};
try {
  "string".search (o);
  assert (false);
} catch (e) {
  assert (e === "abrupt @@search")
}

o = {
  exec: function () { return {index: "Duck"}; },
};
assert ("string".search (o) === 1);

o[Symbol.search] = RegExp.prototype[Symbol.search];
assert ("string".search (o) === "Duck");

o = {
  lastIndex: "Duck",
  exec: () => {
    return "Duck";
  }
}

try {
  RegExp.prototype[Symbol.search].call (o, "Duck");
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

o = {
  exec: () => {
    return { 0: "Duck", index: 0 };
  },
  get lastIndex () {
    return "Duck";
  },
  set lastIndex (v) {
    return "Duck";
  }
}

assert (RegExp.prototype[Symbol.search].call (o, "str") === 0);
