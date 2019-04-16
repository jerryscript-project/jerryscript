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

/* ES2015 24.2.2.1.1 */
try {
  DataView ();
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

/* ES2015 24.2.2.1.2 */
try {
  new DataView (5);
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

/* ES2015 24.2.2.1.3 */
try {
  new DataView ({});
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}

var buffer = new ArrayBuffer (16);

/* ES2015 24.2.2.1.6 */
try {
  new DataView (buffer, { toString: function () { throw new ReferenceError ('foo') } });
  assert (false);
} catch (e) {
  assert (e instanceof ReferenceError);
  assert (e.message === 'foo');
}

/* ES2015 24.2.2.1.7 (numberOffset != offset)*/
try {
  new DataView (buffer, 1.5);
  assert (false);
} catch (e) {
  assert (e instanceof RangeError);
}


/* ES2015 24.2.2.1.7 (offset < 0) */
try {
  new DataView (buffer, -1);
  assert (false);
} catch (e) {
  assert (e instanceof RangeError);
}

/* ES2015 24.2.2.1.10 */
try {
  new DataView (buffer, 17);
  assert (false);
} catch (e) {
  assert (e instanceof RangeError);
}

/* ES2015 24.2.2.1.12.b */
try {
  new DataView (buffer, 0, { toString: function () { throw new ReferenceError ('bar') } });
  assert (false);
} catch (e) {
  assert (e instanceof ReferenceError);
  assert (e.message === 'bar');
}

/* ES2015 24.2.2.1.12.b */
try {
  new DataView (buffer, 0, Infinity);
  assert (false);
} catch (e) {
  assert (e instanceof RangeError);
}

/* ES2015 24.2.2.1.12.c */
try {
  new DataView (buffer, 4, 13);
  assert (false);
} catch (e) {
  assert (e instanceof RangeError);
}

/* Tests accessors: ES2015 24.2.2.1 - 24.2.2.3 */
var accessorList = ['buffer', 'byteOffset', 'byteLength'];

accessorList.forEach (function (prop) {
  /* ES2015 24.2.4.{1, 2, 3}.{2, 3} */
  try {
    var obj = {};
    Object.setPrototypeOf (obj, DataView.prototype);
    obj[prop];
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }
});

buffer = new ArrayBuffer (32);
var view1 = new DataView (buffer);

assert (view1.buffer === buffer);
assert (view1.byteOffset === 0);
assert (view1.byteLength === buffer.byteLength);

var view2 = new DataView (buffer, 8);
assert (view2.buffer === buffer);
assert (view2.byteOffset === 8);
assert (view2.byteLength === buffer.byteLength - view2.byteOffset);

var view3 = new DataView (buffer, 8, 16);
assert (view3.buffer === buffer);
assert (view3.byteOffset === 8);
assert (view3.byteLength === 16);

/* Test get/set routines */
var getters = ['getInt8', 'getUint8', 'getInt16', 'getUint16', 'getInt32', 'getUint32', 'getFloat32', 'getFloat64'];
var setters = ['setInt8', 'setUint8', 'setInt16', 'setUint16', 'setInt32', 'setUint32', 'setFloat32', 'setFloat64'];
var gettersSetters = getters.concat (setters);

gettersSetters.forEach (function (propName) {
  /* ES2015 24.2.1.{1, 2}.1 */
  var routine = DataView.prototype[propName];
  try {
    DataView.prototype[propName].call (5);
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  /* ES2015 24.2.1.{1, 2}.2 */
  try {
    DataView.prototype[propName].call ({});
    assert (false);
  } catch (e) {
    assert (e instanceof TypeError);
  }

  /* ES2015 24.2.1.{1, 2}.5 */
  try {
    var buffer = new ArrayBuffer (16);
    var view = new DataView (buffer)
    view[propName] ({ toString : function () { throw new ReferenceError ('fooBar') } });
    assert (false);
  } catch (e) {
    assert (e instanceof ReferenceError);
    assert (e.message == 'fooBar');
  }

  var buffer = new ArrayBuffer (16);
  var view = new DataView (buffer)

  /* ES2015 24.2.1.{1, 2}.6 (numberIndex != getIndex) */
  try {
    view[propName] (1.5);
    assert (false);
  } catch (e) {
    assert (e instanceof RangeError);
  }

  /* ES2015 24.2.1.{1, 2}.6 (getIndex < 0) */
  try {
    view[propName] (-1);
    assert (false);
  } catch (e) {
    assert (e instanceof RangeError);
  }

  /* ES2015 24.2.1.{1, 2}.13 */
  try {
    view[propName] (20);
    assert (false);
  } catch (e) {
    assert (e instanceof RangeError);
  }
});

/* Test the endianness */
function validateResult (view, offset, isLitteEndian, results) {
  for (var i = 0; i < getters.length; i++) {
    assert (results[i] === view[getters[i]](offset, isLitteEndian));
  }
}

buffer = new ArrayBuffer (24);
view1 = new DataView (buffer);
view2 = new DataView (buffer, 4, 12);
view3 = new DataView (buffer, 6, 18);

view1.setUint8 (0, 255);
validateResult(view1, 0, false, [-1, 255, -256, 65280, -16777216, 4278190080, -1.7014118346046924e+38, -5.486124068793689e+303]);
validateResult(view1, 0, true, [-1, 255, 255, 255, 255, 255, 3.5733110840282835e-43, 1.26e-321]);
validateResult(view1, 2, false, [0, 0, 0, 0, 0, 0, 0, 0]);
validateResult(view1, 2, true, [0, 0, 0, 0, 0, 0, 0, 0]);

view1.setInt16 (4, 40000);
validateResult(view1, 4, false, [-100, 156, -25536, 40000, -1673527296, 2621440000, -6.352747104407253e-22, -1.2938158758247024e-172]);
validateResult(view2, 0, false, [-100, 156, -25536, 40000, -1673527296, 2621440000, -6.352747104407253e-22, -1.2938158758247024e-172]);
validateResult(view1, 4, true, [-100, 156, 16540, 16540, 16540, 16540, 2.3177476599932474e-41, 8.172e-320]);
validateResult(view2, 0, true, [-100, 156, 16540, 16540, 16540, 16540, 2.3177476599932474e-41, 8.172e-320]);

view2.setUint32 (2, 3000000000, true);
validateResult(view2, 2, false, [0, 0, 94, 94, 6213810, 6213810, 8.707402410606192e-39, 6.856613170926581e-307]);
validateResult(view3, 0, false, [0, 0, 94, 94, 6213810, 6213810, 8.707402410606192e-39, 6.856613170926581e-307]);
validateResult(view2, 2, true, [0, 0, 24064, 24064, -1294967296, 3000000000, -2.425713319098577e-8, 1.4821969375e-314]);
validateResult(view3, 0, true, [0, 0, 24064, 24064, -1294967296, 3000000000, -2.425713319098577e-8, 1.4821969375e-314]);

view3.setFloat32 (4, 8.5);
validateResult(view3, 4, false, [65, 65, 16648, 16648, 1091043328, 1091043328, 8.5, 196608]);
validateResult(view3, 4, true, [65, 65, 2113, 2113, 2113, 2113, 2.9609436551183385e-42, 1.044e-320]);
validateResult(view2, 4, false, [-48, 208, -12110, 53426, -793624312, 3501342984, -23924850688, -5.411000515087672e+80]);
validateResult(view2, 4, true, [-48, 208, -19760, 45776, 138523344, 138523344, 5.828901796903399e-34, 6.84396254e-316]);
