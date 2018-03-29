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

(function () {
  var global = (new Function('return this;'))();

  // Define garbage collection handler at highest possible scope as
  // a pure function. Defining it in an inner scope has a high risk
  // of accidentally creating retain cycles...
  function _jerryhandleGarbageCollected(internalProps) {
    console.log('GCd:', internalProps);
    delete __jerry._jerryToHostValueMap[internalProps.jerry_value];
    Module.ccall(
      '_jerry_call_native_object_free_callbacks',
      null, ['number', 'number', 'number', 'number'],
      [internalProps.nativeInfo, internalProps.nativePtr,
        internalProps.nativeHandleFreeCb, internalProps.nativeHandle]);
  };

  var isValueNonPrimitive = function (value) {
    if (value === null) {
      return false;
    }
    var typeStr = typeof value;
    return (typeStr === 'object' || typeStr === 'function');
  };

  var hasInternalProps = function (value) {
    return (isValueNonPrimitive(value) && __jerry._jerryInternalPropsWeakMap.has(value));
  };

  global.__jerry = {
    // Define a WeakMap to maintain the map between JS obj and its internal props
    _jerryInternalPropsWeakMap: new WeakMap(),

    // _jerryToHostValueMap is a 'map' with entry objects that represent what
    // values the native side is holding a reference to (reference count is positive).
    // The _jerryToHostValueMap which will allow us to store and retrieve javascript
    // values from a jerry_value_t, and perform refcounts on values that we still need an
    // internal reference to, and avoid them from being garbage collected.
    _jerryToHostValueMap: {},

    // _hostPrimitiveValueToEntryMap maps primitives (anything that's not an object/function)
    // to an entry object. Because primitives are always leaked (the 'weak' module does not
    // support registering a garbage collection callback for these types), entries in this
    // map are never deleted. The information in this map is redundant to _jerryToHostValueMap.
    // This map is kept to make the lookup of primitives be O(1).
    _hostPrimitiveValueToEntryMap: new Map(),

    // Counter for the next jerry_value_t value.
    _nextObjectRef: 1,

    // the jobqueue list for Promise
    _jobQueue: [],

    _getEntry: function (jerry_value) {
      var entry = this._jerryToHostValueMap[jerry_value];
      if (!entry) {
        console.log(new Error('Entry at ' + jerry_value + ' does not exist').stack);
        throw new Error('Entry at ' + jerry_value + ' does not exist');
      }
      return entry;
    },

    _registerHostValue: function (value, pre_existing_jerry_value) {
      var jerry_value = pre_existing_jerry_value || this._nextObjectRef++;
      // check this._nextObjectRef doesn't overflow the uint32_t type
      // that is used on the C side.
      if (this._nextObjectRef >= Math.pow(2, 32)) {
        throw new Error('The uint32_t jerry_value overflows!');
      }
      var internalProps = {
        jerry_value: jerry_value
      };
      // Store the internalProps, so we can get the same jerry_value,
      // native handle/pointer, etc. later on, even though it was no longer in
      // the _jerryToHostValueMap (because the native side no longer retained it).
      // We can only do it for object/function values and not for primitives.
      // Primitives will always be kept in _jerryToHostValueMap (and leaked)
      // as a work-around, see this.release()
      var nonPrimitive = isValueNonPrimitive(value);
      if (nonPrimitive) {
        if (this._jerryInternalPropsWeakMap.has(value)) {
          // Use pre-existing internalProps:
          internalProps = this._jerryInternalPropsWeakMap.get(value);
        } else {
          this._jerryInternalPropsWeakMap.set(value, internalProps);
        }
      }
      var entry = {
        jerry_value: jerry_value,
        refCount: 1,
        value: value,
        entryForError: null,
        internalProps: internalProps
      };
      // Install garbage collection callback:
      entry.weak = this.weak(value, _jerryhandleGarbageCollected.bind(this, internalProps));
      this._jerryToHostValueMap[jerry_value] = entry;
      if (!nonPrimitive) {
        this._hostPrimitiveValueToEntryMap.set(value, entry);
      }
      // console.log('created entry ' + jerry_value + ' for ' + value + ' at ' + stackTrace());
      return jerry_value;
    },

    _addRefCount: function (entry) {
      if (entry.refCount === 0) {
        entry.value = this.strong(entry.weak);
      }
      entry.refCount++;
    },

    _minusRefCount: function (entry) {
      if (entry.refCount === 0) {
        throw new Error('Deref a value whose refCount is 0!');
      }
      entry.refCount--;
      if (entry.refCount === 0) {
        entry.value = entry.weak;
        entry.entryForError = null;
      }
    },

    reset: function () {
      this._jerryToHostValueMap = {};
      this._hostPrimitiveValueToEntryMap = new Map();
      this._nextObjectRef = 1;
      this._jerryInternalPropsWeakMap = new WeakMap();
      this._jobQueue = [];

      Module.Promise._setAsap(function(callback, arg) {
        __jerry._jobQueue.push({arg: arg, cb: callback});

        // If developer impl the promiseQueuePushedCallback,
        // it will be called whenever a new promise job is pushed into the queue.
        if (Module.promiseQueuePushedCallback) {
          Module.promiseQueuePushedCallback();
        }
      });
    },

    runJobQueue: function () {
      var job = this._jobQueue.shift();
      var ret;
      while (job) {
        try {
          ret = job.cb.call(undefined, job.arg);
        } catch (e) {
          return __jerry.setErrorByValue(e);
        }
        job = this._jobQueue.shift();
      }

      return  __jerry.ref(ret);
    },

    // Given a jerry value, return the stored javascript value.
    get: function (jerry_value) {
      return this._getEntry(jerry_value).value;
    },

    // Given a javascript value, return a jerry value that refers to it.
    // If the value already exists in the map, increment its refcount and return
    // the jerry value.
    // Otherwise, create a new entry and return the jerry value.
    ref: function (value) {
      var jerry_value;
      var entry;
      // Fast path for already known/marked objects:
      if (isValueNonPrimitive(value)) {
        if (this._jerryInternalPropsWeakMap.has(value)) {
          jerry_value = this._jerryInternalPropsWeakMap.get(value).jerry_value;
          // Note: It's possible there is no entry in the map because the native
          // side did not retain it any more, we need to register it again in that case.
          entry = this._jerryToHostValueMap[jerry_value];
        }
      } else {
        // For primitive type
        entry = this._hostPrimitiveValueToEntryMap.get(value);
      }

      if (entry) {
        this._addRefCount(entry);
        return jerry_value || entry.jerry_value;
      }
      return this._registerHostValue(value, jerry_value);
    },

    getRefCount: function (jerry_value) {
      return this._getEntry(jerry_value).refCount;
    },

    // Increase the reference count of the given jerry value
    acquire: function (jerry_value) {
      this._addRefCount(this._getEntry(jerry_value));
      return jerry_value;
    },

    // Decrease the reference count of the given jerry value and delete it if
    // there are no more internal references.
    release: function (ref) {
      var entry = this._getEntry(ref);
      this._minusRefCount(entry);
    },

    // Set or clear the error flag
    // If set: create a new entry whose entryForError point to the original one, return the new entry
    // If clear: disconnect the error link, return the original entry
    setError: function (ref, state) {
      var isError = this.isError(ref);
      if (state === isError) {
        return ref;
      }

      var entry = this._getEntry(ref);
      if (state) {
        var newRef = this._registerHostValue({}, null);
        this._getEntry(newRef).entryForError = entry;
        return newRef;
      } else {
        var oldObj = entry.entryForError.value;
        var oldRef = this.ref(oldObj);
        entry.entryForError = null;
        return oldRef
      }
    },

    // Set the error flag for certain value, and return the error ref.
    setErrorByValue: function (value) {
      var errorRef = __jerry.ref(value);
      var newRef = __jerry.setError(errorRef, true);
      __jerry.release(errorRef);
      return newRef;
    },

    // Whether it is an error
    isError: function (ref) {
      return (this._getEntry(ref).entryForError !== null);
    },

    getRefFromError: function (ref) {
      if (this.isError(ref)) {
        return this.ref(this._getEntry(ref).entryForError.value);
      } else {
        return this.acquire(ref);
      }
    },

    getArgValueRef: function (ref) {
      if (this.isError(ref)) {
        var newRef = this.ref(this._getEntry(ref).entryForError.value);
        this.release(newRef);
        return newRef;
      } else {
        return ref;
      }
    },

    setNativeHandle: function (jerryValue, nativeHandlePtr, freeCallbackPtr) {
      var value = this._getEntry(jerryValue).value;
      if (typeof value !== 'object' && typeof value !== 'function') {
        // jerry_set_object_native_handle is a no-op
        return;
      }
      // The handle and free callback are stored in a property on the value
      // itself. We can't store them in the _jerryToHostValueMap/ref because these only
      // contain
      value.__jerryNativeHandle = {
        nativeHandlePtr: nativeHandlePtr,
        freeCallbackPtr: freeCallbackPtr
      };
    },

    getNativeHandle: function (jerryValue) {
      return this._getEntry(jerryValue).nativeHandlePtr;
    },

    /* Tests whether the host engine supports the __proto__ property to get/set the [[Prototype]] */
    hasProto: ({__proto__: []} instanceof Array),

    /* EM_ASM doesn't allow double quotes nor escape sequences, so define the "use strict" comment string here. */
    getUseStrictComment: function (shouldUseStrict) {
      return shouldUseStrict ? '"use strict";\n' : '';
    },

    /* EM_ASM chokes on object literals... :( */
    typedArrayConstructorByTypeNameMap: {
       // These values MUST match the jerry_typedarray_type_t enum in jerryscript-core.h:
       1: Uint8Array,          /* JERRY_TYPEDARRAY_UINT8, */
       2: Uint8ClampedArray,   /* JERRY_TYPEDARRAY_UINT8CLAMPED, */
       3: Int8Array,           /* JERRY_TYPEDARRAY_INT8, */
       4: Uint16Array,         /* JERRY_TYPEDARRAY_UINT16, */
       5: Int16Array,          /* JERRY_TYPEDARRAY_INT16, */
       6: Uint32Array,         /* JERRY_TYPEDARRAY_UINT32, */
       7: Int32Array,          /* JERRY_TYPEDARRAY_INT32, */
       8: Float32Array,        /* JERRY_TYPEDARRAY_FLOAT32, */
       9: Float64Array,        /* JERRY_TYPEDARRAY_FLOAT64, */
    },

    create_external_function: function (function_ptr) {
      var f = function () {
        var nativeHandlerArgs = [
          function_ptr, /* the function pointer for us to call */
          __jerry.ref(f), /* ref to the actual js function */
          __jerry.ref(this) /* our this object */
        ];

        var numArgs = arguments.length;
        var jsRefs = [];
        for (var i = 0; i < numArgs; i++) {
          jsRefs.push(__jerry.ref(arguments[i]));
        }

        // Arg 4 is a uint32 array of jerry_value_t arguments
        var jsArgs = Module._malloc(numArgs * 4);
        for (var i = 0; i < numArgs; i++) {
          Module.setValue(jsArgs + i * 4, jsRefs[i], 'i32');
        }
        nativeHandlerArgs.push(jsArgs);
        nativeHandlerArgs.push(numArgs);

        // this is just the classy Emscripten calling. function_ptr is a C-pointer here
        // and we know the signature of the C function as it needs to follow
        var result_ref = Module.ccall('_jerry_call_external_handler',
          'number',
          ['number', 'number', 'number', 'number', 'number'],
          nativeHandlerArgs);

        // Free and release all js args
        Module._free(jsArgs);
        while (jsRefs.length > 0) {
          __jerry.release(jsRefs.pop());
        }

        // decrease refcount of native handler arguments
        __jerry.release(nativeHandlerArgs[1]); // jsFunctionRef
        __jerry.release(nativeHandlerArgs[2]); // our this object

        // delete native handler arguments
        nativeHandlerArgs.length = 0;

        var result_val = __jerry.get(result_ref);
        var has_error = __jerry.isError(result_ref);

        if (has_error) {
          var throw_obj = __jerry.get(__jerry.getRefFromError(result_ref));
          __jerry.release(result_ref);
          throw throw_obj;
        }
        __jerry.release(result_ref);
        return result_val;
      };

      return __jerry.ref(f);
    }
  };

  // See if Node.js and 'weak' module is available
  try {
    var weakModule = require('weak');
    __jerry.weak = function (obj, gcCallback) {
      if (obj === null) {
        return obj;
      }
      var typeStr = typeof obj;
      // The `weak` module only works for objects, not for primitive values.
      if (typeStr !== 'object' && typeStr !== 'function') {
        return obj;
      }
      return weakModule(obj, gcCallback);
    };
    __jerry.strong = function (obj) {
      // only worked for the weak ref

      if (obj === null) {
        return obj;
      }
      var typeStr = typeof obj;
      // The `weak` module only works for objects, not for primitive values.
      if (typeStr !== 'object' && typeStr !== 'function') {
        return obj;
      }

      return weakModule.get(obj);
    }
  }
  catch (e) {
    // When not running in Node.js or the 'weak' module isn't available, just
    // provide a stub. This will cause host JS values that pass in/out of the native side
    // to never be garbage collected. The free callback with jerry_set_object_native_handle
    // and jerry_set_object_native_pointer to never get called.
    console.warn('No `weak` module found in host environment. Will leak memory...')
    __jerry.weak = function (obj, gcCallback) {
      return obj;
    };
    __jerry.strong = function (obj) {
      return obj;
    }
  }
})();
