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

var replace = RegExp.prototype[Symbol.replace];

try {
  replace.call (0, "string", "replace");
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  replace.call (new RegExp(), {
    toString: () => {
      throw "abrupt string"
    }
  }, "replace");
  assert (false);
} catch (e) {
  assert (e === "abrupt string");
}

try {
  replace.call (new RegExp(), "string", {
    toString: () => {
      throw "abrupt replace"
    }
  });
  assert (false);
} catch (e) {
  assert (e === "abrupt replace");
}

try {
  replace.call ({
    get global() {
      throw "abrupt global"
    }
  }, "string", "replace");
  assert (false);
} catch (e) {
  assert (e === "abrupt global");
}

try {
  replace.call ({
    global: true,
    set lastIndex(idx) {
      throw "abrupt lastIndex"
    }
  }, "string", "replace");
  assert (false);
} catch (e) {
  assert (e === "abrupt lastIndex");
}

try {
  replace.call ({
    get exec() {
      throw "abrupt exec"
    }
  }, "string", "replace");
  assert (false);
} catch (e) {
  assert (e === "abrupt exec");
}

try {
  replace.call ({
    exec: RegExp.prototype.exec
  }, "string", "replace");
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  replace.call ({
    exec: 42
  }, "string", "replace");
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  replace.call ({
    exec: () => {
      throw "abrupt exec result"
    }
  }, "string", "replace");
  assert (false);
} catch (e) {
  assert (e === "abrupt exec result");
}

try {
  replace.call ({
    exec: () => {
      return 1
    }
  }, "string", "replace");
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

try {
  replace.call ({
    exec: () => {
      return {
        get length() {
          throw "abrupt result length"
        }
      }
    }
  }, "string", "replace");
  assert (false);
} catch (e) {
  assert (e === "abrupt result length");
}

try {
  replace.call ({
      global: true,
      exec: () => {
        return {
          length: 1,
          get 0() {
            throw "abrupt match"
          }
        }
      }
    },
    "string",
    "replace");
  assert (false);
} catch (e) {
  assert (e === "abrupt match");
}

try {
  replace.call ({
      global: true,
      exec: () => {
        return {
          length: 1,
          get 0() {
            return {
              toString: () => {
                throw "abrupt match toString"
              }
            }
          }
        }
      }
    },
    "string",
    "replace");
  assert (false);
} catch (e) {
  assert (e === "abrupt match toString");
}

var result_obj = {
  toString: () => {
    Object.defineProperty (result_obj, 'toString', {
      value: () => {
        throw "abrupt match toString delayed";
      }
    });
    return "str";
  }
}

var first = true;
try {
  replace.call ({
      global: true,
      exec: () => {
        if (!first) {
          return null;
        }

        first = false;
        return {
          length: 1,
          get 0() {
            return result_obj;
          }
        }
      }
    },
    "string",
    "replace");
  assert (false);
} catch (e) {
  assert (e === "abrupt match toString delayed");
}

try {
  replace.call ({
      global: true,
      get lastIndex() {
        throw "abrupt lastIndex get"
      },
      set lastIndex(i) {},
      exec: () => {
        return {
          length: 1,
          get 0() {
            return {
              toString: () => {
                return ""
              }
            }
          }
        }
      }
    },
    "string",
    "replace");
  assert (false);
} catch (e) {
  assert (e === "abrupt lastIndex get");
}

try {
  replace.call ({
      global: true,
      get lastIndex() {
        return {
          valueOf: () => {
            throw "abrupt lastIndex toNumber"
          }
        }
      },
      set lastIndex(i) {},
      exec: () => {
        return {
          length: 1,
          get 0() {
            return {
              toString: () => {
                return ""
              }
            }
          }
        }
      }
    },
    "string",
    "replace");
  assert (false);
} catch (e) {
  assert (e === "abrupt lastIndex toNumber");
}

var o = {
  global: true,
  exec: () => {
    return {
      length: 1,
      get 0() {
        return {
          toString: () => {
            return ""
          }
        }
      }
    }
  }
}
Object.defineProperty (o, 'lastIndex', {
  configurable: true,
  get: () => {
    Object.defineProperty (o, 'lastIndex', {
      get: () => {
        return {
          valueOf: () => {
            return 42
          }
        };
      },
      set: (i) => {
        throw "abrupt lastIndex put";
      },
      configurable: true
    });
    return {
      valueOf: () => {
        return 24
      }
    };
  },
  set: (i) => {}
});

try {
  replace.call (o,
    "string",
    "replace");
  assert (false);
} catch (e) {
  assert (e === "abrupt lastIndex put");
}

o = {
  global: true,
  exec: () => {
    return {
      length: 1,
      get 0() {
        return {
          toString: () => {
            return ""
          }
        }
      }
    }
  },
};
Object.defineProperty (o, 'lastIndex', {
  get: () => {
    Object.defineProperty (o, 'lastIndex', {
      value: 0,
      writable: false
    });
    return 0;
  },
  set: () => {}
});

try {
  replace.call (o,
    "string",
    "replace");
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

o = {
  global: true
};
Object.defineProperty (o, 'exec', {
  configurable: true,
  value: () => {
    Object.defineProperty (o, 'exec', {
      get: () => {
        throw "abrupt exec"
      },
      set: (v) => {}
    });
    return {
      length: 1,
      0: "thisisastring"
    }
  }
});

try {
  replace.call (o,
    "string",
    "replace");
  assert (false);
} catch (e) {
  assert (e === "abrupt exec");
}

try {
  replace.call ({
    exec: () => {
      return {
        length: 1,
        0: "str",
        get index() {
          throw "abrupt index"
        }
      }
    }
  }, "string", "replace");
  assert (false);
} catch (e) {
  assert (e === "abrupt index");
}

try {
  replace.call ({
    exec: () => {
      return {
        length: 1,
        0: "str",
        get index() {
          return {
            valueOf: () => {
              throw "abrupt index toNumber"
            }
          }
        }
      }
    }
  }, "string", "replace");
  assert (false);
} catch (e) {
  assert (e === "abrupt index toNumber");
}

try {
  replace.call ({
    exec: () => {
      return {
        length: 2,
        0: "str",
        index: 0,
        get 1() {
          throw "abrupt capture"
        }
      }
    }
  }, "string", "replace");
  assert (false);
} catch (e) {
  assert (e === "abrupt capture");
}

try {
  replace.call ({
    exec: () => {
      return {
        length: 2,
        0: "str",
        index: 0,
        1: {
          toString: () => {
            throw "abrupt capture toString"
          }
        }
      }
    }
  }, "string", "replace");
  assert (false);
} catch (e) {
  assert (e === "abrupt capture toString");
}

try {
  replace.call ({
    exec: () => {
      return {
        length: 2,
        0: "str",
        index: 0,
        1: "st"
      }
    }
  }, "string", () => {
    throw "abrupt replace"
  });
  assert (false);
} catch (e) {
  assert (e === "abrupt replace");
}

try {
  replace.call ({
    exec: () => {
      return {
        length: 2,
        0: "str",
        index: 0,
        1: "st"
      }
    }
  }, "string", () => {
    return {
      toString: () => {
        throw "abrupt replace toString"
      }
    }
  });
  assert (false);
} catch (e) {
  assert (e === "abrupt replace toString");
}

try {
  replace.call (/abc/, "abc", () => {
    throw "fastpath abrupt replace"
  });
  assert (false);
} catch (e) {
  assert (e === "fastpath abrupt replace");
}

try {
  replace.call (/abc/, "abc", () => {
    return {
      toString: () => {
        throw "fastpath abrupt replace"
      }
    }
  });
  assert (false);
} catch (e) {
  assert (e === "fastpath abrupt replace");
}

assert (replace.call (/abc/, "abc", "xyz") === "xyz");
assert (replace.call (/(c)((d)|(x))(e)/, "abcdefg", "xyz") === "abxyzfg");
assert (replace.call (/(c)((d)|(x))(e)/, "abcdefg", "-$$-") === "ab-$-fg");
assert (replace.call (/(c)((d)|(x))(e)/, "abcdefg", "-$&-") === "ab-cde-fg");
assert (replace.call (/(c)((d)|(x))(e)/, "abcdefg", "-$`-") === "ab-ab-fg");
assert (replace.call (/(c)((d)|(x))(e)/, "abcdefg", "-$'-") === "ab-fg-fg");
assert (replace.call (/(c)((d)|(x))(e)/, "abcdefg", "-$0-") === "ab-$0-fg");
assert (replace.call (/(c)((d)|(x))(e)/, "abcdefg", "-$1-") === "ab-c-fg");
assert (replace.call (/(c)((d)|(x))(e)/, "abcdefg", "-$2-") === "ab-d-fg");
assert (replace.call (/(c)((d)|(x))(e)/, "abcdefg", "-$3-") === "ab-d-fg");
assert (replace.call (/(c)((d)|(x))(e)/, "abcdefg", "-$4-") === "ab--fg");
assert (replace.call (/(c)((d)|(x))(e)/, "abcdefg", "-$5-") === "ab-e-fg");
assert (replace.call (/(c)((d)|(x))(e)/, "abcdefg", "-$6-") === "ab-$6-fg");
assert (replace.call (/(c)((d)|(x))(e)/, "abcdefg", "-$00-") === "ab-$00-fg");
assert (replace.call (/(c)((d)|(x))(e)/, "abcdefg", "-$01-") === "ab-c-fg");
assert (replace.call (/(c)((d)|(x))(e)/, "abcdefg", "-$10-") === "ab-c0-fg");
assert (replace.call (/(c)((d)|(x))(e)/, "abcdefg", "-$99-") === "ab-$99-fg");
assert (replace.call (/(c)((d)|(x))(e)/, "abcdefg", "-$$1-") === "ab-$1-fg");
assert (replace.call (/(c)((d)|(x))(e)/, "abcdefg", "$") === "ab$fg");
assert (replace.call (/(c)((d)|(x))(e)/, "abcdefg", "$@") === "ab$@fg");

replace.call (/(c)((d)|(x))(e)/, "abcdefg", function () {
  assert (arguments[0] === "cde");
  assert (arguments[1] === "c");
  assert (arguments[2] === "d");
  assert (arguments[3] === "d");
  assert (arguments[4] === undefined);
  assert (arguments[5] === "e");
  assert (arguments[6] === 2);
  assert (arguments[7] === "abcdefg");
});

var re = /ab/g;
assert (replace.call (re, "-ab-ab-ab-ab-", "cd") === "-cd-cd-cd-cd-");
assert (re.lastIndex === 0);

re.lastIndex = 5;
assert (replace.call (re, "-ab-ab-ab-ab-", "cd") === "-cd-cd-cd-cd-");
assert (re.lastIndex === 0);

assert (replace.call (/(?:)/g, "string", "Duck") === "DucksDucktDuckrDuckiDucknDuckgDuck");

class Regexplike {
  constructor() {
    this.index = 0;
    this.global = true;
  }

  exec() {
    if (this.index > 0) {
      return null;
    }

    this.index = 39;
    var result = {
      length: 1,
      0: "Duck",
      index: this.index
    };
    return result;
  }
}

re = new Regexplike();

/* Well-behaved RegExp-like object. */
assert (replace.call (re, "What have you brought upon this cursed land", "$&") === "What have you brought upon this cursed Duck");

var replace_count = 0;

function replacer() {
  replace_count++;
  return arguments[0];
}

re.index = 0;
re.exec = function () {
  if (this.index > 3) {
    return null;
  }

  var result = {
    length: 1,
    0: "Duck",
    index: this.index++
  };
  return result;
}

/* Mis-behaving RegExp-like object, replace function is called on each match, but the result is ignored for inconsistent matches. */
assert (replace.call (re, "Badger", replacer) === "Ducker");
assert (replace_count === 4);

re.index = 0;
assert (replace.call (re, "Badger", "Ord") === "Order");

try {
  replace.call (RegExp.prototype, "string", "replace");
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

assert(replace.call({ exec : ( ) => { return {  } } }, 'һ', "a") === "a");
assert(replace.call({ exec : ( ) => { return {  } } }, 'һһһһһһһһһ', "a") === "a");
assert(replace.call({ exec : ( ) => { return {  } } }, 'һһһһһһһһһһ', "a") === "aһ");

/* Object with custom @@replace method */
var o = {}
o[Symbol.replace] = function () {
  return "Duck"
};
assert ("string".replace (o, "Mallard") === "Duck");

o[Symbol.replace] = 42;
try {
  "string".replace (o, "Duck");
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

Object.defineProperty (o, Symbol.replace, {
  get: () => {
    throw "abrupt @@replace get"
  },
  set: (v) => {}
});

try {
  "string".replace (o, "Duck");
  assert (false);
} catch (e) {
  assert (e === "abrupt @@replace get");
}

o = {};
o[Symbol.replace] = function () {
  throw "abrupt @@replace"
};
try {
  "string".replace (o, "str");
  assert (false);
} catch (e) {
  assert (e === "abrupt @@replace")
}

class Regexplike2 {
    exec() {
        return {}
    }
}
re = new Regexplike2();
assert (replace.call (re, "1") === "undefined");

var abruptStickyRegexp = /./;
Object.defineProperty(abruptStickyRegexp, 'sticky', {
  get: function() {
    throw "abrupt sticky";
  }
});

assert(abruptStickyRegexp[Symbol.replace]() === "undefinedndefined");

var r = /./;
r.lastIndex = {
  valueOf: function() {
    throw "abrupt lastIndex"
  }
}

try {
  r[Symbol.replace]("a", "b");
  assert(false);
} catch (e) {
  assert(e === "abrupt lastIndex");
}

var r = /a/y;
r.lastIndex = 3;
assert (r[Symbol.replace]("aaaaa", "b") === "aaaba");
assert (r.lastIndex === 4);

assert (r[Symbol.replace]("ccccc", "b") === "ccccc");
assert (r.lastIndex === 0);

var r = /a/yg;
r.lastIndex = 3;
assert (r[Symbol.replace]("aaaaa", "b") === "bbbbb");
assert (r.lastIndex === 0);

var replaceCalled = false;
var r = /a/
r.lastIndex = 2;

assert(r[Symbol.replace]("aaaa", function(match, index) {
  replaceCalled = true;
  assert (match === "a");
  assert (index === 0);
  return "b";
}) === "baaa");

assert (replaceCalled);
assert (r.lastIndex === 2);

(function () {
  var r = /./g;
  var execWasCalled = false;
  var coercibleIndex = {
    valueOf: function() {
      return Math.pow(2, 54);
    },
  };

  var result = {
    length: 1,
    0: '',
    index: 0,
  };

  r.exec = function() {
    if (execWasCalled) {
      return null;
    }

    r.lastIndex = coercibleIndex;
    execWasCalled = true;
    return result;
  };

  assert(r[Symbol.replace]('', '') === '');
  assert(r.lastIndex === Math.pow(2, 53));
})();
