/* Copyright 2016 Pebble Technology Corp.
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

var __jerryRefs = {

  // Jerryscript values (jerry_value_t) are integers which contain information
  // about some javascript value that has been internally created.
  // Create _objMap which will allow us to store and retrieve javascript values
  // from a jerry_value_t, and perform refcounts on values that we still need an
  // internal reference to, and avoid them from being garbage collected.

  _objMap : {},
  _nextObjectRef : 1,
  _findValue : function(value) {
    if (Number.isNaN(value)) {
      // Special case to find NaN
      for (var jerry_val in this._objMap) {
        if (Number.isNaN(this._objMap[jerry_val].value)) {
          return jerry_val;
        }
      }
    } else {
      for (var jerry_val in this._objMap) {
        if (this._objMap[jerry_val].value === value) {
          return jerry_val;
        }
      }
    }
    return 0;
  },
  _getEntry : function(jerry_value) {
    var entry = this._objMap[jerry_value];
    if (!entry) {
      throw new Error('Entry at ' + jerry_value + ' does not exist');
    }
    return entry;
  },

  reset: function() {
    this._objMap = {};
    this._nextObjectRef = 1;
  },

  // Given a jerry value, return the stored javascript value.
  get : function(jerry_value) {
    return this._getEntry(jerry_value).value;
  },

  // Given a javascript value, return a jerry value that refers to it.
  // If the value already exists in the map, increment its refcount and return
  // the jerry value.
  // Otherwise, create a new entry and return the jerry value.
  ref : function(value) {
    var jerry_value = this._findValue(value);
    if (jerry_value) {
      this._getEntry(jerry_value).refCount++;
      return jerry_value;
    }

    jerry_value = this._nextObjectRef++;
    this._objMap[jerry_value] = {
      refCount : 1,
      value : value,
      error : false,
    };
    // console.log('created entry ' + jerry_value + ' for ' + value + ' at ' + stackTrace());
    return jerry_value;
  },

  getRefCount : function(jerry_value) {
    return this._getEntry(jerry_value).refCount;
  },

  // Increase the reference count of the given jerry value
  acquire : function(jerry_value) {
    this._getEntry(jerry_value).refCount++;
    return jerry_value;
  },

  // Decrease the reference count of the given jerry value and delete it if
  // there are no more internal references.
  release : function(ref) {
    var entry = this._getEntry(ref);
    entry.refCount--;

    if (entry.refCount <= 0) {
      if (entry.freeCallbackPtr) {
        Module.ccall(
          'emscripten_call_jerry_object_free_callback',
          null,
          ['number', 'number'],
          [entry.freeCallbackPtr, entry.nativeHandlePtr]);
      }
      // console.log('deleting ' + ref + ' at ' + stackTrace());
      delete this._objMap[ref];
    }
  },

  setError : function(ref, state) {
    this._getEntry(ref).error = state;
  },

  getError : function(ref) {
    return this._getEntry(ref).error;
  },

  setNativeHandle : function(jerryValue, nativeHandlePtr, freeCallbackPtr) {
    var entry = this._getEntry(jerryValue);
    entry.nativeHandlePtr = nativeHandlePtr;
    entry.freeCallbackPtr = freeCallbackPtr;
  },

  getNativeHandle : function(jerryValue) {
    return this._getEntry(jerryValue).nativeHandlePtr;
  }
};

function __jerry_create_external_function(function_ptr) {
  var f = function() {
    var nativeHandlerArgs = [
        function_ptr, /* the function pointer for us to call */
        __jerryRefs.ref(f), /* ref to the actual js function */
        __jerryRefs.ref(this) /* our this object */
    ];

    var numArgs = arguments.length;
    var jsRefs = [];
    for (var i = 0; i < numArgs; i++) {
      jsRefs.push(__jerryRefs.ref(arguments[i]));
    }

    // Arg 4 is a uint32 array of jerry_value_t arguments
    var jsArgs = Module._malloc(numArgs * 4);
    for (var i = 0; i < numArgs; i++) {
      Module.setValue(jsArgs + i*4, jsRefs[i], 'i32');
    }
    nativeHandlerArgs.push(jsArgs);
    nativeHandlerArgs.push(numArgs);

    // this is just the classy Emscripten calling. function_ptr is a C-pointer here
    // and we know the signature of the C function as it needs to follow
    var result_ref = Module.ccall('emscripten_call_jerry_function',
        'number',
        ['number', 'number', 'number', 'number', 'number'],
        nativeHandlerArgs);

    // Free and release all js args
    Module._free(jsArgs);
    while (jsRefs.length > 0) {
      __jerryRefs.release(jsRefs.pop());
    }

    // decrease refcount of native handler arguments
    __jerryRefs.release(nativeHandlerArgs[1]); // jsFunctionRef
    __jerryRefs.release(nativeHandlerArgs[2]); // our this object

    // delete native handler arguments
    nativeHandlerArgs.length = 0;

    var result_val = __jerryRefs.get(result_ref);
    var has_error = __jerryRefs.getError(result_ref);
    __jerryRefs.release(result_ref);

    if (has_error) {
      throw result_val;
    }

    return result_val;
  };

  return __jerryRefs.ref(f);
}
