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

// helper function - simple implementation
Array.prototype.equals = function (array) {
  if (this.length != array.length)
    return false;

  for (var i = 0; i < this.length; i++) {
    if (this[i] instanceof Array && array[i] instanceof Array) {
      if (!this[i].equals (array[i]))
        return false;
      }
      else if (this[i] != array[i]) {
        return false;
    }
  }

  return true;
}

// converting map to object
const map = new Map ([ ['foo', 'bar'], ['baz', 42] ]);
const obj_map = Object.fromEntries (map);
assert (Object.values (obj_map).equals (["bar", 42]));
assert (Object.keys (obj_map).equals (["foo", 'baz']));

// converting array to object
const arr = [ ['0', 'a'], ['1', 'b'], ['2', 'c'] ];
const obj_arr = Object.fromEntries (arr);
assert (Object.values (obj_arr).equals (["a", "b", "c"]));
assert (Object.keys (obj_arr).equals (["0", '1', '2']));

// transfrom object to other object
const object1 = { a: 1, b: 2, c: 3 };
const object2 = Object.fromEntries (
  Object.entries (object1)
  .map (([ key, val ]) => [ key, val * 2 ])
);
assert (Object.keys (object2).equals (["a", "b", "c"]));
assert (Object.values (object2).equals ([2, 4, 6]));

// map with undefined or null member
const map2 = new Map ([ ['foo', undefined], ['baz', null] ]);
const obj_map2 = Object.fromEntries (map2);
assert (Object.values (obj_map2).equals ([undefined, null]));
assert (Object.keys(obj_map2).equals (["foo", 'baz']))

// don't have a value
const map3 = new Map ([ ['foo'], ['baz'] ]);
const obj_map3 = Object.fromEntries (map3);
assert (Object.values(obj_map3).equals ([undefined, undefined]));
assert (Object.keys(obj_map3).equals (["foo", 'baz']));

// empty map
const map4 = new Map ([]);
const obj_map4 = Object.fromEntries (map4);
assert (Object.values(obj_map4).equals ([]));
assert (Object.keys(obj_map4).equals ([]));

// few invalid argument
function check_iterator (iterator) {
  try {
    Object.fromEntries (iterator);
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }
}

check_iterator (null);
check_iterator (undefined);
check_iterator (5);
check_iterator ()

// closed iterator
var returned = false;
var closed_iterable = {
  [Symbol.iterator]: function () {
    var advanced = false;
    return {
      next: function () {
        if (advanced) {
          throw 42 // meaning of life;
        }
        advanced = true;
        return {
          done: false,
          value: 'ab',
        };
      },
      return: function () {
        if (returned) {
          throw 42 // meaning of life;
        }
        returned = true;
      },
    };
  },
};

check_iterator (closed_iterable)
assert (returned);

var next_iterable = {
  [Symbol.iterator]: function () {
    return {
      next: function () {
        return null;
      },
      return: function () {
        throw 42 // meaning of life;
      },
    };
  },
};

check_iterator (next_iterable)

// uncallable next
var next_iterable_2 = {
  [Symbol.iterator]: function () {
    return {
      next: null,
      return: function () {
        throw 42 // meaning of life;
      },
    };
  },
};

check_iterator (next_iterable_2)

// get '0' error
returned = false;
var iterable_0 = {
  [Symbol.iterator]: function () {
    var advanced = false;
    return {
      next: function () {
        if (advanced) {
          throw 42 // meaning of life;
        }
        advanced = true;
        return {
          done: false,
          value: {
            get '0' () {
              throw new TypeError ();
            },
            get '1' () {
              return "value";
            },
          },
        };
      },
      return: function () {
        if (returned) {
          throw 42 // meaning of life;
        }
        returned = true;
      },
    };
  },
};

check_iterator (iterable_0)
assert (returned);

// error in toPropertyKey
returned = false;
var iterable = {
  [Symbol.iterator]: function () {
    var advanced = false;
    return {
      next: function () {
        if (advanced) {
          throw 42 // meaning of life;
        }
        advanced = true;
        return {
          done: false,
          value: {
            0: {
              get toString () { throw new TypeError }
            },
            get '1' () {
              return "value";
            },
          },
        };
      },
      return: function () {
        if (returned) {
          throw 42 // meaning of life;
        }
        returned = true;
      },
    };
  },
};

check_iterator (iterable)
assert (returned);

// get '1' error
returned = false;
var iterable = {
  [Symbol.iterator]: function () {
    var advanced = false;
    return {
      next: function () {
        if (advanced) {
          throw 42 // meaning of life;
        }
        advanced = true;
        return {
          done: false,
          value: {
            get '0' () {
              return 'key';
            },
            get '1' () {
              throw new TypeError;
            },
          },
        };
      },
      return: function () {
        if (returned) {
          throw 42 // meaning of life;
        }
        returned = true;
      },
    };
  },
};

check_iterator (iterable)
assert (returned);

// next value is error
var iterable = {
  [Symbol.iterator] () {
    return {
      next () {
        return {
          get value () {
            throw new TypeError }
          }
        }
      }
    }
  }

check_iterator (iterable)
