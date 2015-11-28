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

// Checking quoting strings
assert (JSON.stringify ("") === '""');

normal_string = "asdasd";
assert (JSON.stringify (normal_string) == '"asdasd"');

format_characters = "\ba\fs\nd\ra\tsd";
assert (JSON.stringify (format_characters) == '"\\ba\\fs\\nd\\ra\\tsd"');

ctl_string = "asdasd";
assert (JSON.stringify (ctl_string) == '"asd\\u001fasd"');

escpad_string = "\"asda\sd";
assert (JSON.stringify (escpad_string) == '"\\"asdasd"');

assert (JSON.stringify('\u2040') == '"⁀"');
assert (JSON.stringify('abc\u2040\u2030cba') == '"abc⁀‰cba"');

// Checking primitive types
assert (JSON.stringify (1) === '1');
assert (JSON.stringify (true) === 'true');
assert (JSON.stringify ("foo") === '"foo"');
assert (JSON.stringify (null) === 'null');
assert (JSON.stringify (undefined) === undefined);

assert (JSON.stringify (new Number(1)) === '1');
assert (JSON.stringify (new Boolean(true)) === 'true');
assert (JSON.stringify (new String("foo")) === '"foo"');

// Checking objects
empty_object = {}
assert (JSON.stringify (empty_object) == '{}');

empty_object = {};
empty_object.a = undefined;

assert (JSON.stringify (empty_object) == '{}');

p_object = { "a": 1, "b": true, "c": "foo", "d": null, "e": undefined };
assert (JSON.stringify (p_object) == '{"a":1,"b":true,"c":"foo","d":null}');

o_object = { "a": new Number(1), "b": new Boolean(true), "c": new String("foo") };
assert (JSON.stringify (o_object) == '{"a":1,"b":true,"c":"foo"}');

child = { "a": 1, "b": new String("\nfoo"), "c": undefined };
parent = { "a": true, "b": child, "c": null};

assert (JSON.stringify (parent) == '{"a":true,"b":{"a":1,"b":"\\nfoo"},"c":null}');

recursive_object = {};
recursive_object.a = 2;
recursive_object.b = recursive_object;

try {
  JSON.stringify (recursive_object)
  // Should not be reached.
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

// Checking arrays
empty_array = [];
assert (JSON.stringify (empty_array) == '[]');

array = [undefined];
assert (JSON.stringify (array) == '[null]');

p_array = [1, true, "foo", null, undefined];
assert (JSON.stringify (p_array) == '[1,true,"foo",null,null]');

o_array = [new Number(1), new Boolean(true), new String("foo")];
assert (JSON.stringify (o_array) == '[1,true,"foo"]');

child = [ 1, new String("\nfoo"), undefined ];
parent = [ true, child, null ];

assert (JSON.stringify (parent) == '[true,[1,"\\nfoo",null],null]');

recursive_array = [];
recursive_array[0] = 2;
recursive_array[1] = recursive_array;

try {
  JSON.stringify (recursive_array)
  // Should not be reached.
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

object = {"a": 1, "b": [1, true, {"a": "foo"}]};
assert (JSON.stringify (object) == '{"a":1,"b":[1,true,{"a":"foo"}]}');

object = {"a": [1], "b": {}};
assert (JSON.stringify(object) === '{"a":[1],"b":{}}');

array = [1, {"a": 2, "b": true, c: [3]}];
assert (JSON.stringify (array) == '[1,{"a":2,"b":true,"c":[3]}]');

// Filtering / replacing
to_json_object = {};
to_json_object.a = 2;
to_json_object.toJSON = function (key) { return 3; };

assert (JSON.stringify (to_json_object) === "3");

function replacer_function (key, value)
{
  if (typeof(value) == "string")
    return "FOO";
  return value;
}

object = { "a": "JSON", "b": new String("JSON"), "c": 3 };
assert (JSON.stringify (object, replacer_function) == '{"a":"FOO","b":"JSON","c":3}');

filter = ["a", "b"];
assert (JSON.stringify (object, filter) == '{"a":"JSON","b":"JSON"}');

assert (JSON.stringify ([], [ 'foo', 'foo' ]) === '[]');

number = new Number(2.2);
number.toString = {};
number.valueOf = [];

try
{
  JSON.stringify([], [number]);
  // Should not be reached.
  assert (false);
}
catch (e)
{
  assert (e instanceof TypeError);
}

// Throw error in the replacer function
function replacer_thrower (key, value)
{
  throw new ReferenceError("foo");
  return value;
}

try {
  JSON.stringify (object, replacer_thrower)
  // Should not be reached.
  assert (false);
} catch (e) {
  assert (e.message === "foo");
  assert (e instanceof ReferenceError);
}

// Checking replacer with different primitive types
object = { "a": 2 };
assert (JSON.stringify (object, 3) == '{"a":2}');
assert (JSON.stringify (object, true) == '{"a":2}');
assert (JSON.stringify (object, null) == '{"a":2}');
assert (JSON.stringify (object, undefined) == '{"a":2}');
assert (JSON.stringify (object, "foo") == '{"a":2}');

// Checking replacer with different primitive types
assert (JSON.stringify (object, new Boolean (true)) == '{"a":2}');
assert (JSON.stringify (object, new Number (3)) == '{"a":2}');
assert (JSON.stringify (object, new String ("foo")) == '{"a":2}');
assert (JSON.stringify (object, { "a": 3 }) == '{"a":2}');

// Checking JSON formatting
object = {"a": 2};
assert (JSON.stringify (object, null, "   ") == '{\n   "a": 2\n}');
assert (JSON.stringify (object, null, "asd") == '{\nasd"a": 2\n}');
assert (JSON.stringify (object, null, "asd0123456789") == '{\nasd0123456"a": 2\n}');
assert (JSON.stringify (object, null, "asd\u20400123456789") == '{\nasd⁀012345"a": 2\n}');
assert (JSON.stringify (object, null, 100) == '{\n          "a": 2\n}');
assert (JSON.stringify (object, null, -5) == '{"a":2}');

array = [2];
assert (JSON.stringify (array, null, "   ") == '[\n   2\n]');
assert (JSON.stringify (array, null, "asd") == '[\nasd2\n]');
assert (JSON.stringify (array, null, "asd0123456789") == '[\nasd01234562\n]');
assert (JSON.stringify (array, null, "asd\u20400123456789") == '[\nasd⁀0123452\n]');
assert (JSON.stringify (array, null, 100) == '[\n          2\n]');
assert (JSON.stringify (array, null, -5) == '[2]');

nested_object = {"a": 2, "b": {"c": 1, "d": true}};
assert (JSON.stringify (nested_object, null, 2) == '{\n  "a": 2,\n  "b": {\n    "c": 1,\n    "d": true\n  }\n}');

nested_array = [2, [1,true]];
assert (JSON.stringify (nested_array, null, 2) == '[\n  2,\n  [\n    1,\n    true\n  ]\n]');

// Checking space (formatting parameter) with different primititve types
object = { "a": 2 };
assert (JSON.stringify (object, null, true) == '{"a":2}');
assert (JSON.stringify (object, null, null) == '{"a":2}');
assert (JSON.stringify (object, null, undefined) == '{"a":2}');

// Checking space (formatting parameter) with different object types
assert (JSON.stringify (object, null, new Boolean (true)) == '{"a":2}');
assert (JSON.stringify (object, null, [1, 2, 3] ) == '{"a":2}');
assert (JSON.stringify (object, null, { "a": 3 }) == '{"a":2}');
