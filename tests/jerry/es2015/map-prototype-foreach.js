/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

var map = new Map();

/* Test when the map object keys are strings */
for (var i = 0; i < 15; i++) {
  map.set ("foo" + i, i);
  assert (map.size === i + 1);
}

var counter = 0;

map.forEach (function (value, key) {
  assert (typeof key === "string");
  assert (key === "foo" + counter);
  assert (typeof value === "number");
  assert (value === counter);
  counter++;
});

map.clear ();
assert (map.size === 0);
counter = 0;

/* Test when the map object keys are numbers */
for (var i = 0; i < 15; i++) {
  map.set (i + 0.5, "foo" + i);
  assert (map.size === i + 1);
}

map.forEach (function (value, key) {
  assert (typeof key === "number");
  assert (key === counter + 0.5);
  assert (typeof value === "string");
  assert (value === "foo" + counter);
  counter++;
});

map.clear ();
assert (map.size === 0);
counter = 0;

var objectList = [];

/* Test when the map object keys are objects */
for (var i = 0; i < 15; i++) {
  objectList[i] = { index : i };
  map.set (objectList[i], "foo" + i);
  assert (map.size === i + 1);
}

map.forEach (function (value, key) {
  assert (typeof key === "object");
  assert (key === objectList[counter]);
  assert (key.index === objectList[counter].index);
  assert (typeof value === "string");
  assert (value === "foo" + counter);
  counter++;
});

/* Test when the this argument is not a map object */
try {
  Map.prototype.forEach.call ({});
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

/* Test when the 1st argument is not callable */
try {
  map.forEach (5);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

/* Test when the the callback function throws error */
try {
  map.forEach (function (value, key) {
    throw new ReferenceError ("foo");
  });
  assert (false);
} catch (e) {
  assert (e instanceof ReferenceError);
  assert (e.message === "foo");
}

/* Test when the 2nd optional argument is present */
var object = {
  _secret : 42
}

map.forEach (function (value, key) {
  assert (this._secret === 42);
}, object);
