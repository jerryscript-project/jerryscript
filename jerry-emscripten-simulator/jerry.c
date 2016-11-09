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

#include "jerry-api.h"

#include <emscripten/emscripten.h>

#include <string.h>

#define TYPE_ERROR \
    jerry_create_error(JERRY_ERROR_TYPE, NULL);

#define TYPE_ERROR_ARG \
    jerry_create_error( \
      JERRY_ERROR_TYPE, (const jerry_char_t *)"wrong type of argument");

#define TYPE_ERROR_FLAG \
    jerry_create_error( \
        JERRY_ERROR_TYPE, \
        (const jerry_char_t *)"argument cannot have an error flag");

////////////////////////////////////////////////////////////////////////////////
// Parser and Executor Function
////////////////////////////////////////////////////////////////////////////////

// Note that `is_strict` is currently unsupported by emscripten
jerry_value_t jerry_eval(const jerry_char_t *source_p, size_t source_size,
                         bool is_strict) {
  // FIXME: use is_strict
  (void)is_strict;

  return (jerry_value_t)EM_ASM_INT({
      // jerry_eval() uses an indirect eval() call,
      // so the global execution context is used.
      // Also see ECMA 5.1 -- 10.4.2 Entering Eval Code.
      var indirectEval = eval;
      try {
        return __jerryRefs.ref(indirectEval(Module.Pointer_stringify($0, $1)));
      } catch (e) {
        var error_ref = __jerryRefs.ref(e);
        __jerryRefs.setError(error_ref, true);
        return error_ref;
      }
    }, source_p, source_size);
}

size_t jerry_parse_and_save_snapshot(const jerry_char_t *source_p,
                                     size_t source_size,
                                     bool is_for_global, bool is_strict,
                                     uint8_t *buffer_p, size_t buffer_size) {
  EM_ASM(throw new Error('Not implemented'));
  (void)source_p;
  (void)source_size;
  (void)is_for_global;
  (void)is_strict;
  (void)buffer_p;
  (void)buffer_size;
  return 0;
}

jerry_value_t jerry_parse(const jerry_char_t *source_p, size_t source_size,
                          bool is_strict) {
  EM_ASM(throw new Error('Not implemented'));
  (void)source_p;
  (void)source_size;
  (void)is_strict;
  return 0;
}

jerry_value_t jerry_run(const jerry_value_t func_val) {
  EM_ASM(throw new Error('Not implemented'));
  (void)func_val;
  return 0;
}

jerry_value_t jerry_exec_snapshot(const void *snapshot_p, size_t snapshot_size,
                                  bool copy_bytecode) {
  EM_ASM(throw new Error('Not implemented'));
  (void)snapshot_p;
  (void)snapshot_size;
  (void)copy_bytecode;
  return 0;
}

jerry_value_t jerry_acquire_value(jerry_value_t value) {
  return (jerry_value_t)EM_ASM_INT({
      return __jerryRefs.acquire($0);
    }, value);
}

void jerry_release_value(jerry_value_t value) {
  EM_ASM_INT({
      __jerryRefs.release($0);
    }, value);
}

////////////////////////////////////////////////////////////////////////////////
// Get the global context
////////////////////////////////////////////////////////////////////////////////
jerry_value_t jerry_get_global_object(void) {
  return ((jerry_value_t)EM_ASM_INT_V({ \
      return __jerryRefs.ref(Function('return this;')()); \
    }));
}

jerry_value_t jerry_get_global_builtin(const jerry_char_t *builtin_name) {
  return ((jerry_value_t)EM_ASM_INT({ \
      var global = Function('return this;')();
      return __jerryRefs.ref(global[Module.Pointer_stringify($0)]); \
    }, builtin_name));
}

////////////////////////////////////////////////////////////////////////////////
// Jerry Value Type Checking
////////////////////////////////////////////////////////////////////////////////

#define JERRY_VALUE_HAS_TYPE(ref, typename) \
    ((bool)EM_ASM_INT({ \
             return typeof __jerryRefs.get($0) === (typename); \
           }, (ref)))

#define JERRY_VALUE_IS_INSTANCE(ref, type) \
    ((bool)EM_ASM_INT({ \
             return __jerryRefs.get($0) instanceof (type); \
           }, (ref)))

bool jerry_value_is_array(const jerry_value_t value) {
  return JERRY_VALUE_IS_INSTANCE(value, Array);
}

bool jerry_value_is_boolean(const jerry_value_t value) {
  return JERRY_VALUE_HAS_TYPE(value, 'boolean');
}

bool jerry_value_is_constructor(const jerry_value_t value) {
  return jerry_value_is_function(value);
}

bool jerry_value_is_function(const jerry_value_t value) {
  return JERRY_VALUE_HAS_TYPE(value, 'function');
}

bool jerry_value_is_number(const jerry_value_t value) {
  return JERRY_VALUE_HAS_TYPE(value, 'number');
}

bool jerry_value_is_null(const jerry_value_t value) {
  return ((bool)EM_ASM_INT({
      return __jerryRefs.get($0) === null;
    }, value));
}

bool jerry_value_is_object(const jerry_value_t value) {
  return !jerry_value_is_null(value) &&
          (JERRY_VALUE_HAS_TYPE(value, 'object') ||
           jerry_value_is_function(value));
}

bool jerry_value_is_string(const jerry_value_t value) {
  return JERRY_VALUE_HAS_TYPE(value, 'string');
}

bool jerry_value_is_undefined(const jerry_value_t value) {
  return JERRY_VALUE_HAS_TYPE(value, 'undefined');
}

////////////////////////////////////////////////////////////////////////////////
// Jerry Value Getter Functions
////////////////////////////////////////////////////////////////////////////////

bool jerry_get_boolean_value(const jerry_value_t value) {
  if (!jerry_value_is_boolean(value)) {
    return false;
  }
  return (bool)EM_ASM_INT({
      return (__jerryRefs.get($0) === true);
    }, value);
}

double jerry_get_number_value(const jerry_value_t value) {
  if (!jerry_value_is_number(value)) {
    return 0.0;
  }
  return EM_ASM_DOUBLE({
      return __jerryRefs.get($0);
    }, value);
}
////////////////////////////////////////////////////////////////////////////////
// Functions for UTF-8 encoded string values
////////////////////////////////////////////////////////////////////////////////
jerry_size_t jerry_get_utf8_string_size(const jerry_value_t value) {
  if (!jerry_value_is_string(value)) {
    return 0;
  }
  return (jerry_size_t)EM_ASM_INT({
      return Module.lengthBytesUTF8(__jerryRefs.get($0));
    }, value);
}

jerry_size_t jerry_string_to_utf8_char_buffer(const jerry_value_t value,
                                              jerry_char_t *buffer_p,
                                              jerry_size_t buffer_size) {
  const jerry_size_t str_size = jerry_get_utf8_string_size(value);
  if (str_size == 0 || buffer_size < str_size || buffer_p == NULL) {
    return 0;
  }

  EM_ASM_INT({
      var str = __jerryRefs.get($0);
      // Add one onto the buffer size, since Module.stringToUTF8 adds a null
      // character at the end. This will lead to truncation if we just use
      // buffer_size. Since the actual jerry-api does not do this, we are
      // always careful to allocate space for a null character at the end.
      // Allow stringToUTF8 to write that extra null beyond the passed in
      // buffer_length.
      Module.stringToUTF8(str, $1, $2 + 1);
    }, value, buffer_p, buffer_size);
  return strlen((const char *)buffer_p);
}

////////////////////////////////////////////////////////////////////////////////
// Functions for array object values
////////////////////////////////////////////////////////////////////////////////
uint32_t jerry_get_array_length(const jerry_value_t value) {
  if (!jerry_value_is_array(value)) {
    return 0;
  }
  return (uint32_t)EM_ASM_INT({
      return __jerryRefs.get($0).length;
    }, value);
}

////////////////////////////////////////////////////////////////////////////////
// Jerry Value Creation API
////////////////////////////////////////////////////////////////////////////////
#define JERRY_CREATE_VALUE(value) \
    ((jerry_value_t)EM_ASM_INT_V({ \
        return __jerryRefs.ref((value)); \
      }))

jerry_value_t jerry_create_array(uint32_t size) {
  return (jerry_value_t)EM_ASM_INT({
      return __jerryRefs.ref(new Array($0));
    }, size);
}

jerry_value_t jerry_create_boolean(bool value) {
  return (jerry_value_t)EM_ASM_INT({
      return __jerryRefs.ref(Boolean($0));
    }, value);
}

jerry_value_t jerry_create_error(jerry_error_t error_type,
                                 const jerry_char_t *message_p) {
  return jerry_create_error_sz(error_type,
                               message_p, strlen((const char *)message_p));
}

#define JERRY_ERROR(type, msg, sz) (jerry_value_t)(EM_ASM_INT({ \
      return __jerryRefs.ref(new (type)(Module.Pointer_stringify($0, $1))) \
    }, (msg), (sz)))

jerry_value_t jerry_create_error_sz(jerry_error_t error_type,
                                    const jerry_char_t *message_p,
                                    jerry_size_t message_size) {
  jerry_value_t error_ref = 0;
  switch (error_type) {
    case JERRY_ERROR_COMMON:
      error_ref = JERRY_ERROR(Error, message_p, message_size);
      break;
    case JERRY_ERROR_EVAL:
      error_ref = JERRY_ERROR(EvalError, message_p, message_size);
      break;
    case JERRY_ERROR_RANGE:
      error_ref = JERRY_ERROR(RangeError, message_p, message_size);
      break;
    case JERRY_ERROR_REFERENCE:
      error_ref = JERRY_ERROR(ReferenceError, message_p, message_size);
      break;
    case JERRY_ERROR_SYNTAX:
      error_ref = JERRY_ERROR(SyntaxError, message_p, message_size);
      break;
    case JERRY_ERROR_TYPE:
      error_ref = JERRY_ERROR(TypeError, message_p, message_size);
      break;
    case JERRY_ERROR_URI:
      error_ref = JERRY_ERROR(URIError, message_p, message_size);
      break;
    default:
      EM_ASM_INT({
          abort('Cannot create error type: ' + $0);
        }, error_type);
      break;
  }
  jerry_value_set_error_flag(&error_ref);
  return error_ref;
}

jerry_value_t jerry_create_external_function(jerry_external_handler_t handler_p) {
  return (jerry_value_t)EM_ASM_INT({
      return __jerry_create_external_function($0);
    }, handler_p);
}

jerry_value_t jerry_create_number(double value) {
  return (jerry_value_t)EM_ASM_INT({
      return __jerryRefs.ref($0);
    }, value);
}

jerry_value_t jerry_create_number_infinity(bool negative) {
  if (negative) {
    return JERRY_CREATE_VALUE(-Infinity);
  } else {
    return JERRY_CREATE_VALUE(Infinity);
  }
}

jerry_value_t jerry_create_number_nan(void) {
  return JERRY_CREATE_VALUE(NaN);
}

jerry_value_t jerry_create_null(void) {
  return JERRY_CREATE_VALUE(null);
}

jerry_value_t jerry_create_object(void) {
  return JERRY_CREATE_VALUE(new Object());
}

jerry_value_t jerry_create_string_sz(const jerry_char_t *str_p,
                                     jerry_size_t str_size) {
  if (!str_p) {
    return jerry_create_undefined();
  }
  return (jerry_value_t)EM_ASM_INT({
      return __jerryRefs.ref(Module.Pointer_stringify($0, $1));
    }, str_p, str_size);
}

jerry_value_t jerry_create_string(const jerry_char_t *str_p) {
  if (!str_p) {
    return jerry_create_undefined();
  }
  return jerry_create_string_sz(str_p, strlen((const char *)str_p));
}

jerry_size_t jerry_get_string_size(const jerry_value_t value) {
  if (!jerry_value_is_string(value)) {
    return 0;
  }
  return (jerry_size_t)EM_ASM_INT({
      return Module.lengthBytesUTF8(__jerryRefs.get($0));
    }, value);
}

jerry_value_t jerry_create_undefined(void) {
  return JERRY_CREATE_VALUE(undefined);
}

////////////////////////////////////////////////////////////////////////////////
// General API Functions of JS Objects
////////////////////////////////////////////////////////////////////////////////

bool jerry_has_property(const jerry_value_t obj_val,
                        const jerry_value_t prop_name_val) {
  if (!jerry_value_is_object(obj_val) ||
      !jerry_value_is_string(prop_name_val)) {
    return false;
  }
  return (bool)EM_ASM_INT({
      var obj = __jerryRefs.get($0);
      var name = __jerryRefs.get($1);
      return (name in obj);
    }, obj_val, prop_name_val);
}

bool jerry_has_own_property(const jerry_value_t obj_val,
                            const jerry_value_t prop_name_val) {
  if (!jerry_value_is_object(obj_val) ||
      !jerry_value_is_string(prop_name_val)) {
    return false;
  }
  return (bool)EM_ASM_INT({
      var obj = __jerryRefs.get($0);
      var name = __jerryRefs.get($1);
      return obj.hasOwnProperty(name);
    }, obj_val, prop_name_val);
}

bool jerry_delete_property(const jerry_value_t obj_val,
                           const jerry_value_t prop_name_val) {
  if (!jerry_value_is_object(obj_val) ||
      !jerry_value_is_string(prop_name_val)) {
    return false;
  }
  return (bool)EM_ASM_INT({
      var obj = __jerryRefs.get($0);
      var name = __jerryRefs.get($1);
      try {
        return delete obj[name];
      } catch (e) {
        // In strict mode, delete throws SyntaxError if the property is an
        // own non-configurable property.
        return false;
      }
      return true;
    }, obj_val, prop_name_val);
}

jerry_value_t jerry_get_property(const jerry_value_t obj_val,
                                 const jerry_value_t prop_name_val) {
  if (!jerry_value_is_object(obj_val) ||
      !jerry_value_is_string(prop_name_val)) {
    return TYPE_ERROR_ARG;
  }
  return (jerry_value_t)EM_ASM_INT({
      var obj = __jerryRefs.get($0);
      var name = __jerryRefs.get($1);
      return __jerryRefs.ref(obj[name]);
    }, obj_val, prop_name_val);
}

jerry_value_t jerry_get_property_by_index(const jerry_value_t obj_val,
                                          uint32_t index) {
  if (!jerry_value_is_object(obj_val)) {
    return TYPE_ERROR;
  }
  return (jerry_value_t)EM_ASM_INT({
      var obj = __jerryRefs.get($0);
      return __jerryRefs.ref(obj[$1]);
    }, obj_val, index);
}

jerry_value_t jerry_set_property(const jerry_value_t obj_val,
                                 const jerry_value_t prop_name_val,
                                 const jerry_value_t value_to_set) {
  if (jerry_value_has_error_flag(value_to_set) ||
      !jerry_value_is_object(obj_val) ||
      !jerry_value_is_string(prop_name_val)) {
    return TYPE_ERROR_ARG;
  }
  return (jerry_value_t)EM_ASM_INT({
      var obj = __jerryRefs.get($0);
      var name = __jerryRefs.get($1);
      var to_set = __jerryRefs.get($2);
      obj[name] = to_set;
      return __jerryRefs.ref(true);
    }, obj_val, prop_name_val, value_to_set);
}

jerry_value_t jerry_set_property_by_index(const jerry_value_t obj_val,
                                          uint32_t index,
                                          const jerry_value_t value_to_set) {
  if (jerry_value_has_error_flag(value_to_set) ||
      !jerry_value_is_object(obj_val)) {
    return TYPE_ERROR_ARG;
  }
  return (jerry_value_t)EM_ASM_INT({
      var obj = __jerryRefs.get($0);
      var to_set = __jerryRefs.get($2);
      obj[$1] = to_set;
      return __jerryRefs.ref(true);
    }, obj_val, index, value_to_set);
}

void jerry_init_property_descriptor_fields(jerry_property_descriptor_t *prop_desc_p) {
  *prop_desc_p = (jerry_property_descriptor_t) {
    .value = jerry_create_undefined(),
    .getter = jerry_create_undefined(),
    .setter = jerry_create_undefined(),
  };
}

jerry_value_t jerry_define_own_property(const jerry_value_t obj_val,
                                        const jerry_value_t prop_name_val,
                                        const jerry_property_descriptor_t *pdp) {
  if (!jerry_value_is_object(obj_val) && !jerry_value_is_string(obj_val)) {
    return TYPE_ERROR_ARG;
  }
  if ((pdp->is_writable_defined || pdp->is_value_defined)
      && (pdp->is_get_defined || pdp->is_set_defined)) {
    return TYPE_ERROR_ARG;
  }
  if (pdp->is_get_defined && !jerry_value_is_function(pdp->getter)) {
    return TYPE_ERROR_ARG;
  }
  if (pdp->is_set_defined && !jerry_value_is_function(pdp->setter)) {
    return TYPE_ERROR_ARG;
  }

  return (jerry_value_t)(EM_ASM_INT({
      var obj = __jerryRefs.get($12 /* obj_val */);
      var name = __jerryRefs.get($13 /* prop_name_val */);
      var desc = {};
      if ($0 /* is_value_defined */) {
        desc.value = __jerryRefs.get($9);
      }
      if ($1 /* is_get_defined */) {
        desc.get = __jerryRefs.get($10);
      }
      if ($2 /* is_set_defined */) {
        desc.set = __jerryRefs.get($11);
      }
      if ($3 /* is_writable_defined */) {
        desc.writable = Boolean($4 /* is_writable */);
      }
      if ($5 /* is_enumerable_defined */) {
        desc.enumerable = Boolean($6 /* is_enumerable */);
      }
      if ($7 /* is_configurable */) {
        desc.configurable = Boolean($8 /* is_configurable */);
      }

      Object.defineProperty(obj, name, desc);
      return __jerryRefs.ref(Boolean(true));
    }, pdp->is_value_defined,    /* $0 */
       pdp->is_get_defined,      /* $1 */
       pdp->is_set_defined,      /* $2 */
       pdp->is_writable_defined, /* $3 */
       pdp->is_writable,         /* $4 */
       pdp->is_enumerable_defined,   /* $5 */
       pdp->is_enumerable,           /* $6 */
       pdp->is_configurable_defined, /* $7 */
       pdp->is_configurable,         /* $8 */
       pdp->value,   /* $9  */
       pdp->getter,  /* $10 */
       pdp->setter,  /* $11 */
       obj_val,      /* $12 */
       prop_name_val /* $13 */
    ));
}

jerry_value_t emscripten_call_jerry_function(jerry_external_handler_t func_obj_p,
                                             const jerry_value_t func_obj_val,
                                             const jerry_value_t this_val,
                                             const jerry_value_t args_p[],
                                             jerry_size_t args_count) {
  return (func_obj_p)(func_obj_val, this_val, args_p, args_count);
}

jerry_value_t jerry_call_function(const jerry_value_t func_obj_val,
                                  const jerry_value_t this_val,
                                  const jerry_value_t args_p[],
                                  jerry_size_t args_count) {
  if (!jerry_value_is_function(func_obj_val)) {
    return TYPE_ERROR_ARG;
  }

  return (jerry_value_t)EM_ASM_INT({
        var func_obj = __jerryRefs.get($0);
        var this_val = __jerryRefs.get($1);
        var args = [];
        for (var i = 0; i < $3; ++i) {
          args.push(__jerryRefs.get(getValue($2 + i*4, 'i32')));
        }
        try {
          var rv = func_obj.apply(this_val, args);
        } catch (e) {
          var error_ref = __jerryRefs.ref(e);
          __jerryRefs.setError(error_ref, true);
          return error_ref;
        }
        return __jerryRefs.ref(rv);
    }, func_obj_val, this_val, args_p, args_count);
}

jerry_value_t jerry_construct_object(const jerry_value_t func_obj_val,
                                     const jerry_value_t args_p[],
                                     jerry_size_t args_count) {
  if (!jerry_value_is_constructor(func_obj_val)) {
    return TYPE_ERROR_ARG;
  }
  return (jerry_value_t)EM_ASM_INT({
        var func_obj = __jerryRefs.get($0);
        var args = [];
        for (var i = 0; i < $2; ++i) {
          args.push(__jerryRefs.get(getValue($1 + i*4, 'i32')));
        }
        // Call the constructor with new object as `this`
        var bindArgs = [null].concat(args);
        var boundConstructor = func_obj.bind.apply(func_obj, bindArgs);
        var rv = new boundConstructor();
        return __jerryRefs.ref(rv);
    }, func_obj_val, args_p, args_count);
}

jerry_size_t jerry_string_to_char_buffer(const jerry_value_t value,
                                         jerry_char_t *buffer_p,
                                         jerry_size_t buffer_size) {
  return jerry_string_to_utf8_char_buffer(value, buffer_p, buffer_size);
}

jerry_size_t jerry_object_to_string_to_utf8_char_buffer(const jerry_value_t object,
                                                        jerry_char_t *buffer_p,
                                                        jerry_size_t buffer_size) {
  jerry_value_t str_ref = (jerry_value_t)EM_ASM_INT({
    var str = __jerryRefs.ref(String(__jerryRefs.get($0)));
    return str;
  }, object);
  jerry_size_t len = jerry_string_to_utf8_char_buffer(str_ref, buffer_p, buffer_size);
  jerry_release_value(str_ref);

  return len;
}

// FIXME: Propery do CESU-8 => UTF-8 conversion.
jerry_size_t jerry_object_to_string_to_char_buffer(const jerry_value_t object,
                                                   jerry_char_t *buffer_p,
                                                   jerry_size_t buffer_size) {
  return jerry_object_to_string_to_utf8_char_buffer(object,
                                                    buffer_p,
                                                    buffer_size);
}

jerry_value_t jerry_get_object_keys(const jerry_value_t value) {
  if (!jerry_value_is_object(value)) {
    return TYPE_ERROR_ARG;
  }
  return (jerry_value_t)EM_ASM_INT({
      return __jerryRefs.ref(Object.keys(__jerryRefs.get($0)));
    }, value);
}

jerry_value_t jerry_get_prototype(const jerry_value_t value) {
  if (!jerry_value_is_object(value)) {
    return TYPE_ERROR_ARG;
  }
  return (jerry_value_t)EM_ASM_INT({
      return __jerryRefs.ref(__jerryRefs.get($0).prototype);
    }, value);
}

jerry_value_t jerry_set_prototype(const jerry_value_t obj_val,
                                  const jerry_value_t proto_obj_val) {
  // FIXME: Not sure what to do here, perhaps assign __prototype___?
  EM_ASM(throw new Error('Not implemented'));
  (void)obj_val;
  (void)proto_obj_val;
  return 0;
}

bool jerry_get_object_native_handle(const jerry_value_t obj_val,
                                    uintptr_t *out_handle_p) {
  return EM_ASM_INT({
    var ptr = __jerryRefs.getNativeHandle($0);
    if (ptr === undefined) {
      return false;
    }
    Module.setValue($1, ptr, '*');
    return true;
  }, obj_val, out_handle_p);
}

void jerry_set_object_native_handle(const jerry_value_t obj_val,
                                    uintptr_t handle_p,
                                    jerry_object_free_callback_t freecb_p) {
  EM_ASM_INT({
    __jerryRefs.setNativeHandle($0, $1, $2);
  }, obj_val, handle_p, freecb_p);
}

void emscripten_call_jerry_object_free_callback(jerry_object_free_callback_t freecb_p,
                                                uintptr_t handle_p) {
  if (freecb_p) {
    freecb_p(handle_p);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Error flag manipulation functions
////////////////////////////////////////////////////////////////////////////////
//
// The error flag is stored alongside the value in __jerryRefs.
// This allows for us to keep a valid value, like jerryscript does, and be able
// to add / remove a flag specifying whether there was an error or not.

bool jerry_value_has_error_flag(const jerry_value_t value) {
  return (bool)(EM_ASM_INT({
      return __jerryRefs.getError($0);
    }, value));
}

void jerry_value_clear_error_flag(jerry_value_t *value_p) {
  EM_ASM_INT({
      return __jerryRefs.setError($0, false);
    }, *value_p);
}

void jerry_value_set_error_flag(jerry_value_t *value_p) {
  EM_ASM_INT({
      return __jerryRefs.setError($0, true);
    }, *value_p);
}
////////////////////////////////////////////////////////////////////////////////
// Converters of `jerry_value_t`
////////////////////////////////////////////////////////////////////////////////

bool jerry_value_to_boolean(const jerry_value_t value) {
  if (jerry_value_has_error_flag(value)) {
    return false;
  }
  return (bool)EM_ASM_INT({
      return Boolean(__jerryRefs.get($0));
    }, value);
}



jerry_value_t jerry_value_to_number(const jerry_value_t value) {
  if (jerry_value_has_error_flag(value)) {
    return TYPE_ERROR_FLAG;
  }
  return (jerry_value_t)EM_ASM_INT({
      return __jerryRefs.ref(Number(__jerryRefs.get($0)));
    }, value);
}

jerry_value_t jerry_value_to_object(const jerry_value_t value) {
  if (jerry_value_has_error_flag(value)) {
    return TYPE_ERROR_FLAG;
  }
  return (jerry_value_t)EM_ASM_INT({
      return __jerryRefs.ref(new Object(__jerryRefs.get($0)));
    }, value);
}

jerry_value_t jerry_value_to_primitive(const jerry_value_t value) {
  if (jerry_value_has_error_flag(value)) {
    return TYPE_ERROR_FLAG;
  }
  return (jerry_value_t)EM_ASM_INT({
      var val = __jerryRefs.get($0);
      var rv;
      if ((typeof val === 'object' && val != null)
          || (typeof val === 'function')) {
        rv = val.valueOf(); // unbox
      } else {
        rv = val; // already a primitive
      }
      return __jerryRefs.ref(rv);
    }, value);
}

jerry_value_t jerry_value_to_string(const jerry_value_t value) {
  if (jerry_value_has_error_flag(value)) {
    return TYPE_ERROR_FLAG;
  }
  return (jerry_value_t)EM_ASM_INT({
      return __jerryRefs.ref(String(__jerryRefs.get($0)));
    }, value);
}

int jerry_obj_refcount(jerry_value_t o) {
  return EM_ASM_INT({
    try {
      return __jerryRefs.getRefCount($0);
    } catch (e) {
      return 0;
    }
  }, o);
}

void jerry_get_memory_limits(size_t *out_data_bss_brk_limit_p,
                             size_t *out_stack_limit_p) {
  *out_data_bss_brk_limit_p = 0;
  *out_stack_limit_p = 0;
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

void jerry_init(jerry_init_flag_t flags) {
  EM_ASM(__jerryRefs.reset());
  (void)flags;
}

void jerry_cleanup(void) {
}
