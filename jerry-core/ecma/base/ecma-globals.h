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

#ifndef ECMA_GLOBALS_H
#define ECMA_GLOBALS_H

#include "config.h"
#include "jrt.h"
#include "lit-magic-strings.h"
#include "jmem.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmatypes ECMA types
 * @{
 *
 * \addtogroup compressedpointer Compressed pointer
 * @{
 */

/**
 * The NULL value for compressed pointers
 */
#define ECMA_NULL_POINTER JMEM_CP_NULL

/**
 * @}
 */

/**
 * JerryScript init flags.
 */
typedef enum
{
  ECMA_INIT_EMPTY               = (0u),      /**< empty flag set */
  ECMA_INIT_SHOW_OPCODES        = (1u << 0), /**< dump byte-code to log after parse */
  ECMA_INIT_SHOW_REGEXP_OPCODES = (1u << 1), /**< dump regexp byte-code to log after compilation */
  ECMA_INIT_MEM_STATS           = (1u << 2), /**< dump memory statistics */
} ecma_init_flag_t;

/**
 * JerryScript status flags.
 */
typedef enum
{
  ECMA_STATUS_API_AVAILABLE     = (1u << 0), /**< api available */
  ECMA_STATUS_DIRECT_EVAL       = (1u << 1), /**< eval is called directly */
#if ENABLED (JERRY_PROPRETY_HASHMAP)
  ECMA_STATUS_HIGH_PRESSURE_GC  = (1u << 2), /**< last gc was under high pressure */
#endif /* ENABLED (JERRY_PROPRETY_HASHMAP) */
  ECMA_STATUS_EXCEPTION         = (1u << 3), /**< last exception is a normal exception */
} ecma_status_flag_t;

/**
 * Type of ecma value
 */
typedef enum
{
  ECMA_TYPE_DIRECT = 0, /**< directly encoded value, a 28 bit signed integer or a simple value */
  ECMA_TYPE_STRING = 1, /**< pointer to description of a string */
  ECMA_TYPE_FLOAT = 2, /**< pointer to a 64 or 32 bit floating point number */
  ECMA_TYPE_OBJECT = 3, /**< pointer to description of an object */
  ECMA_TYPE_SYMBOL = 4, /**< pointer to description of a symbol */
  ECMA_TYPE_DIRECT_STRING = 5, /**< directly encoded string values */
  ECMA_TYPE_ERROR = 7, /**< pointer to description of an error reference (only supported by C API) */
  ECMA_TYPE_SNAPSHOT_OFFSET = ECMA_TYPE_ERROR, /**< offset to a snapshot number/string */
  ECMA_TYPE___MAX = ECMA_TYPE_ERROR /** highest value for ecma types */
} ecma_type_t;

#if ENABLED (JERRY_DEBUGGER)
/**
 * Shift for scope chain index part in ecma_parse_opts
 */
#define ECMA_PARSE_CHAIN_INDEX_SHIFT 16
#endif /* ENABLED (JERRY_DEBUGGER) */

/**
 * Option flags for script parsing.
 * Note:
 *      The enum members must be kept in sync with parser_general_flags_t
 *      The last 16 bits are reserved for scope chain index
 */
typedef enum
{
  ECMA_PARSE_NO_OPTS = 0, /**< no options passed */
  ECMA_PARSE_STRICT_MODE = (1u << 0), /**< enable strict mode */
  ECMA_PARSE_DIRECT_EVAL = (1u << 1), /**< eval is called directly (ECMA-262 v5, 15.1.2.1.1) */
  /* These four status flags must be in this order. See PARSER_CLASS_PARSE_OPTS_OFFSET. */
  ECMA_PARSE_CLASS_CONSTRUCTOR = (1u << 2), /**< a class constructor is being parsed (this value must be kept in
                                             *   in sync with PARSER_CLASS_CONSTRUCTOR) */
  ECMA_PARSE_HAS_SUPER = (1u << 3), /**< the current context has super reference */
  ECMA_PARSE_HAS_IMPL_SUPER = (1u << 4), /**< the current context has implicit parent class */
  ECMA_PARSE_HAS_STATIC_SUPER = (1u << 5), /**< the current context is a static class method */
  ECMA_PARSE_EVAL = (1u << 6), /**< eval is called */
  ECMA_PARSE_MODULE = (1u << 7), /**< module is parsed */
} ecma_parse_opts_t;

/**
 * Description of an ecma value
 *
 * Bit-field structure: type (3) | value (29)
 */
typedef uint32_t ecma_value_t;

/**
 * Type for directly encoded integer numbers in JerryScript.
 */
typedef int32_t ecma_integer_value_t;

#if UINTPTR_MAX <= UINT32_MAX

/**
 * JMEM_ALIGNMENT_LOG aligned pointers can be stored directly in ecma_value_t
 */
#define ECMA_VALUE_CAN_STORE_UINTPTR_VALUE_DIRECTLY

#endif /* UINTPTR_MAX <= UINT32_MAX */

/**
 * Mask for ecma types in ecma_value_t
 */
#define ECMA_VALUE_TYPE_MASK 0x7u

/**
 * Shift for value part in ecma_value_t
 */
#define ECMA_VALUE_SHIFT 3

/**
 * Mask for directly encoded values
 */
#define ECMA_DIRECT_TYPE_MASK ((1u << ECMA_VALUE_SHIFT) | ECMA_VALUE_TYPE_MASK)

/**
 * Ecma integer value type
 */
#define ECMA_DIRECT_TYPE_INTEGER_VALUE ((0u << ECMA_VALUE_SHIFT) | ECMA_TYPE_DIRECT)

/**
 * Ecma simple value type
 */
#define ECMA_DIRECT_TYPE_SIMPLE_VALUE ((1u << ECMA_VALUE_SHIFT) | ECMA_TYPE_DIRECT)

/**
 * Shift for directly encoded values in ecma_value_t
 */
#define ECMA_DIRECT_SHIFT 4

/**
 * ECMA make simple value
 */
#define ECMA_MAKE_VALUE(value) \
  ((((ecma_value_t) (value)) << ECMA_DIRECT_SHIFT) | ECMA_DIRECT_TYPE_SIMPLE_VALUE)

/**
 * Simple ecma values
 */
enum
{
  /**
   * Empty value is implementation defined value, used for representing:
   *   - empty (uninitialized) values
   *   - immutable binding values
   *   - special register or stack values for vm
   */
  ECMA_VALUE_EMPTY = ECMA_MAKE_VALUE (0), /**< uninitialized value */
  ECMA_VALUE_ERROR = ECMA_MAKE_VALUE (1), /**< an error is currently thrown */
  ECMA_VALUE_FALSE = ECMA_MAKE_VALUE (2), /**< boolean false */
  ECMA_VALUE_TRUE = ECMA_MAKE_VALUE (3), /**< boolean true */
  ECMA_VALUE_UNDEFINED = ECMA_MAKE_VALUE (4), /**< undefined value */
  ECMA_VALUE_NULL = ECMA_MAKE_VALUE (5), /**< null value */
  ECMA_VALUE_ARRAY_HOLE = ECMA_MAKE_VALUE (6), /**< array hole, used for
                                                *   initialization of an array literal */
  ECMA_VALUE_NOT_FOUND = ECMA_MAKE_VALUE (7), /**< a special value returned by
                                               *   ecma_op_object_find */
  ECMA_VALUE_REGISTER_REF = ECMA_MAKE_VALUE (8), /**< register reference,
                                                  *   a special "base" value for vm */
  ECMA_VALUE_IMPLICIT_CONSTRUCTOR = ECMA_MAKE_VALUE (9), /**< special value for bound class constructors */
  ECMA_VALUE_UNINITIALIZED = ECMA_MAKE_VALUE (10), /**< a special value for uninitialized let/const declarations */
};

#if !ENABLED (JERRY_NUMBER_TYPE_FLOAT64)
/**
 * Maximum integer number for an ecma value
 */
#define ECMA_INTEGER_NUMBER_MAX         0x7fffff
/**
 * Maximum integer number for an ecma value (shifted left with ECMA_DIRECT_SHIFT)
 */
#define ECMA_INTEGER_NUMBER_MAX_SHIFTED 0x7fffff0
#else /* ENABLED (JERRY_NUMBER_TYPE_FLOAT64) */
/**
 * Maximum integer number for an ecma value
 */
#define ECMA_INTEGER_NUMBER_MAX         0x7ffffff
/**
 * Maximum integer number for an ecma value (shifted left with ECMA_DIRECT_SHIFT)
 */
#define ECMA_INTEGER_NUMBER_MAX_SHIFTED 0x7ffffff0
#endif /* !ENABLED (JERRY_NUMBER_TYPE_FLOAT64) */

#if !ENABLED (JERRY_NUMBER_TYPE_FLOAT64)
/**
 * Minimum integer number for an ecma value
 */
#define ECMA_INTEGER_NUMBER_MIN         -0x7fffff
/**
 * Minimum integer number for an ecma value (shifted left with ECMA_DIRECT_SHIFT)
 */
#define ECMA_INTEGER_NUMBER_MIN_SHIFTED -0x7fffff0
#else /* ENABLED (JERRY_NUMBER_TYPE_FLOAT64) */
/**
 * Minimum integer number for an ecma value
 */
#define ECMA_INTEGER_NUMBER_MIN         -0x8000000
/**
 * Minimum integer number for an ecma value (shifted left with ECMA_DIRECT_SHIFT)
 */
#define ECMA_INTEGER_NUMBER_MIN_SHIFTED (-0x7fffffff - 1) /* -0x80000000 */
#endif /* !ENABLED (JERRY_NUMBER_TYPE_FLOAT64) */

#if ECMA_DIRECT_SHIFT != 4
#error "Please update ECMA_INTEGER_NUMBER_MIN/MAX_SHIFTED according to the new value of ECMA_DIRECT_SHIFT."
#endif

/**
 * Checks whether the integer number is in the integer number range.
 */
#define ECMA_IS_INTEGER_NUMBER(num) \
  (ECMA_INTEGER_NUMBER_MIN <= (num) && (num) <= ECMA_INTEGER_NUMBER_MAX)

/**
 * Maximum integer number, which if squared, still fits in ecma_integer_value_t
 */
#if !ENABLED (JERRY_NUMBER_TYPE_FLOAT64)
#define ECMA_INTEGER_MULTIPLY_MAX 0xb50
#else /* ENABLED (JERRY_NUMBER_TYPE_FLOAT64) */
#define ECMA_INTEGER_MULTIPLY_MAX 0x2d41
#endif /* !ENABLED (JERRY_NUMBER_TYPE_FLOAT64) */

/**
 * Checks whether the error flag is set.
 */
#define ECMA_IS_VALUE_ERROR(value) \
  (JERRY_UNLIKELY ((value) == ECMA_VALUE_ERROR))

/**
 * Callback which tells whether the ECMAScript execution should be stopped.
 */
typedef ecma_value_t (*ecma_vm_exec_stop_callback_t) (void *user_p);

/**
 * Type of an external function handler.
 */
typedef ecma_value_t (*ecma_external_handler_t) (const ecma_value_t function_obj,
                                                 const ecma_value_t this_val,
                                                 const ecma_value_t args_p[],
                                                 const ecma_length_t args_count);

/**
 * Native free callback of an object.
 */
typedef void (*ecma_object_native_free_callback_t) (void *native_p);

/**
 * Type information of a native pointer.
 */
typedef struct
{
  ecma_object_native_free_callback_t free_cb; /**< the free callback of the native pointer */
} ecma_object_native_info_t;

/**
 * Representation for native pointer data.
 */
typedef struct ecma_native_pointer_t
{
  void *data_p; /**< points to the data of the object */
  ecma_object_native_info_t *info_p; /**< native info */
  struct ecma_native_pointer_t *next_p; /**< points to the next ecma_native_pointer_t element */
} ecma_native_pointer_t;

/**
 * Property list:
 *   The property list of an object is a chain list of various items.
 *   The type of each item is stored in the first byte of the item.
 *
 *   The most common item is the property pair, which contains two
 *   ecmascript properties. It is also important, that after the
 *   first property pair, only property pair items are allowed.
 *
 *   Example for other items is property name hash map, or array of items.
 */

/**
 * Property type list.
 */
typedef enum
{
  ECMA_PROPERTY_TYPE_SPECIAL, /**< special purpose property (deleted / hashmap) */
  ECMA_PROPERTY_TYPE_NAMEDDATA, /**< property is named data */
  ECMA_PROPERTY_TYPE_NAMEDACCESSOR, /**< property is named accessor */
  ECMA_PROPERTY_TYPE_INTERNAL, /**< internal property with custom data field */
  ECMA_PROPERTY_TYPE_VIRTUAL = ECMA_PROPERTY_TYPE_INTERNAL, /**< property is virtual data property */

  ECMA_PROPERTY_TYPE__MAX = ECMA_PROPERTY_TYPE_VIRTUAL, /**< highest value for property types. */
} ecma_property_types_t;

/**
 * Property name listing options.
 */
typedef enum
{
  ECMA_LIST_NO_OPTS = (0), /**< no options are provided */
  ECMA_LIST_ARRAY_INDICES = (1 << 0), /**< exclude properties with names
                                       *   that are not indices */
  ECMA_LIST_ENUMERABLE = (1 << 1), /**< exclude non-enumerable properties */
  ECMA_LIST_PROTOTYPE = (1 << 2), /**< list properties from prototype chain */
#if ENABLED (JERRY_ES2015)
  ECMA_LIST_SYMBOLS = (1 << 3), /**< list symbol properties only */
#endif /* ENABLED (JERRY_ES2015) */
  ECMA_LIST_CONVERT_FAST_ARRAYS = (1 << 4), /**< after listing the properties convert
                                             *   the fast access mode array back to normal array */
} ecma_list_properties_options_t;

/**
 * List enumerable properties and include the prototype chain.
 */
#define ECMA_LIST_ENUMERABLE_PROTOTYPE (ECMA_LIST_ENUMERABLE | ECMA_LIST_PROTOTYPE)

/**
 * Property type mask.
 */
#define ECMA_PROPERTY_TYPE_MASK 0x3

/**
 * Property flags base shift.
 */
#define ECMA_PROPERTY_FLAG_SHIFT 2

/**
 * Property flag list (for ECMA_PROPERTY_TYPE_NAMEDDATA
 * and ECMA_PROPERTY_TYPE_NAMEDACCESSOR).
 */
typedef enum
{
  ECMA_PROPERTY_FLAG_CONFIGURABLE = 1u << (ECMA_PROPERTY_FLAG_SHIFT + 0), /**< property is configurable */
  ECMA_PROPERTY_FLAG_ENUMERABLE = 1u << (ECMA_PROPERTY_FLAG_SHIFT + 1), /**< property is enumerable */
  ECMA_PROPERTY_FLAG_WRITABLE = 1u << (ECMA_PROPERTY_FLAG_SHIFT + 2), /**< property is writable */
  ECMA_PROPERTY_FLAG_LCACHED = 1u << (ECMA_PROPERTY_FLAG_SHIFT + 3), /**< property is lcached */
} ecma_property_flags_t;

/**
 * Property flags configurable, enumerable, writable.
 */
#define ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE_WRITABLE \
  (ECMA_PROPERTY_FLAG_CONFIGURABLE | ECMA_PROPERTY_FLAG_ENUMERABLE | ECMA_PROPERTY_FLAG_WRITABLE)

/**
 * Property flags configurable, enumerable.
 */
#define ECMA_PROPERTY_CONFIGURABLE_ENUMERABLE \
  (ECMA_PROPERTY_FLAG_CONFIGURABLE | ECMA_PROPERTY_FLAG_ENUMERABLE)

/**
 * Property flags configurable, enumerable.
 */
#define ECMA_PROPERTY_CONFIGURABLE_WRITABLE \
  (ECMA_PROPERTY_FLAG_CONFIGURABLE | ECMA_PROPERTY_FLAG_WRITABLE)

/**
 * Property flags enumerable, writable.
 */
#define ECMA_PROPERTY_ENUMERABLE_WRITABLE \
  (ECMA_PROPERTY_FLAG_ENUMERABLE | ECMA_PROPERTY_FLAG_WRITABLE)

/**
 * No attributes can be changed for this property.
 */
#define ECMA_PROPERTY_FIXED 0

/**
 * Shift for property name part.
 */
#define ECMA_PROPERTY_NAME_TYPE_SHIFT (ECMA_PROPERTY_FLAG_SHIFT + 4)

/**
 * Convert data property to internal property.
 */
#define ECMA_CONVERT_DATA_PROPERTY_TO_INTERNAL_PROPERTY(property_p) \
   *(property_p) = (uint8_t) (*(property_p) + (ECMA_PROPERTY_TYPE_INTERNAL - ECMA_PROPERTY_TYPE_NAMEDDATA))

/**
 * Convert internal property to data property.
 */
#define ECMA_CONVERT_INTERNAL_PROPERTY_TO_DATA_PROPERTY(property_p) \
   *(property_p) = (uint8_t) (*(property_p) - (ECMA_PROPERTY_TYPE_INTERNAL - ECMA_PROPERTY_TYPE_NAMEDDATA))

/**
 * Special property identifiers.
 */
typedef enum
{
  /* Note: when new special types are added
   * ECMA_PROPERTY_IS_PROPERTY_PAIR must be updated as well. */
  ECMA_SPECIAL_PROPERTY_HASHMAP, /**< hashmap property */
  ECMA_SPECIAL_PROPERTY_DELETED, /**< deleted property */

  ECMA_SPECIAL_PROPERTY__COUNT /**< Number of special property types */
} ecma_special_property_id_t;

/**
 * Define special property type.
 */
#define ECMA_SPECIAL_PROPERTY_VALUE(type) \
  ((uint8_t) (ECMA_PROPERTY_TYPE_SPECIAL | ((type) << ECMA_PROPERTY_NAME_TYPE_SHIFT)))

/**
 * Type of deleted property.
 */
#define ECMA_PROPERTY_TYPE_DELETED ECMA_SPECIAL_PROPERTY_VALUE (ECMA_SPECIAL_PROPERTY_DELETED)

/**
 * Type of hash-map property.
 */
#define ECMA_PROPERTY_TYPE_HASHMAP ECMA_SPECIAL_PROPERTY_VALUE (ECMA_SPECIAL_PROPERTY_HASHMAP)

/**
 * Type of property not found.
 */
#define ECMA_PROPERTY_TYPE_NOT_FOUND ECMA_PROPERTY_TYPE_HASHMAP

/**
 * Type of property not found and no more searching in the proto chain.
 */
#define ECMA_PROPERTY_TYPE_NOT_FOUND_AND_STOP ECMA_PROPERTY_TYPE_DELETED

/**
 * Abstract property representation.
 *
 * A property is a type_and_flags byte and an ecma_value_t value pair.
 * This pair is represented by a single pointer in JerryScript. Although
 * a packed struct would only consume sizeof(ecma_value_t)+1 memory
 * bytes, accessing such structure is inefficient from the CPU viewpoint
 * because the value is not naturally aligned. To improve performance,
 * two type bytes and values are packed together. The memory layout is
 * the following:
 *
 *  [type 1, type 2, unused byte 1, unused byte 2][value 1][value 2]
 *
 * The unused two bytes are used to store a compressed pointer for the
 * next property pair.
 *
 * The advantage of this layout is that the value reference can be computed
 * from the property address. However, property pointers cannot be compressed
 * anymore.
 */
typedef uint8_t ecma_property_t; /**< ecma_property_types_t (3 bit) and ecma_property_flags_t */

/**
 * Number of items in a property pair.
 */
#define ECMA_PROPERTY_PAIR_ITEM_COUNT 2

/**
 * Property header for all items in a property list.
 */
typedef struct
{
#if ENABLED (JERRY_CPOINTER_32_BIT)
  jmem_cpointer_t next_property_cp; /**< next cpointer */
#endif /* ENABLED (JERRY_CPOINTER_32_BIT) */
  ecma_property_t types[ECMA_PROPERTY_PAIR_ITEM_COUNT]; /**< two property type slot. The first represent
                                                         *   the type of this property (e.g. property pair) */
#if ENABLED (JERRY_CPOINTER_32_BIT)
  uint16_t padding; /**< an unused value */
#else /* !ENABLED (JERRY_CPOINTER_32_BIT) */
  jmem_cpointer_t next_property_cp; /**< next cpointer */
#endif /* ENABLED (JERRY_CPOINTER_32_BIT) */
} ecma_property_header_t;

/**
 * Pair of pointers - to property's getter and setter
 */
typedef struct
{
  jmem_cpointer_t getter_cp; /**< compressed pointer to getter object */
  jmem_cpointer_t setter_cp; /**< compressed pointer to setter object */
} ecma_getter_setter_pointers_t;

/**
 * Property data.
 */
typedef union
{
  ecma_value_t value; /**< value of a property */
#if ENABLED (JERRY_CPOINTER_32_BIT)
  jmem_cpointer_t getter_setter_pair_cp; /**< cpointer to getter setter pair */
#else /* !ENABLED (JERRY_CPOINTER_32_BIT) */
  ecma_getter_setter_pointers_t getter_setter_pair; /**< getter setter pair */
#endif /* ENABLED (JERRY_CPOINTER_32_BIT) */
} ecma_property_value_t;

/**
 * Property pair.
 */
typedef struct
{
  ecma_property_header_t header; /**< header of the property */
  ecma_property_value_t values[ECMA_PROPERTY_PAIR_ITEM_COUNT]; /**< property value slots */
  jmem_cpointer_t names_cp[ECMA_PROPERTY_PAIR_ITEM_COUNT]; /**< property name slots */
} ecma_property_pair_t;

/**
 * Get property type.
 */
#define ECMA_PROPERTY_GET_TYPE(property) \
  ((ecma_property_types_t) ((property) & ECMA_PROPERTY_TYPE_MASK))

/**
 * Get property name type.
 */
#define ECMA_PROPERTY_GET_NAME_TYPE(property) \
  ((property) >> ECMA_PROPERTY_NAME_TYPE_SHIFT)

/**
 * Returns true if the property pointer is a property pair.
 */
#define ECMA_PROPERTY_IS_PROPERTY_PAIR(property_header_p) \
  ((property_header_p)->types[0] != ECMA_PROPERTY_TYPE_HASHMAP)

/**
 * Returns true if the property is named property.
 */
#define ECMA_PROPERTY_IS_NAMED_PROPERTY(property) \
  (ECMA_PROPERTY_GET_TYPE (property) != ECMA_PROPERTY_TYPE_SPECIAL)

/**
 * Add the offset part to a property for computing its property data pointer.
 */
#define ECMA_PROPERTY_VALUE_ADD_OFFSET(property_p) \
  ((uintptr_t) ((((uint8_t *) (property_p)) + (sizeof (ecma_property_value_t) * 2 - 1))))

/**
 * Align the property for computing its property data pointer.
 */
#define ECMA_PROPERTY_VALUE_DATA_PTR(property_p) \
  (ECMA_PROPERTY_VALUE_ADD_OFFSET (property_p) & ~(sizeof (ecma_property_value_t) - 1))

/**
 * Compute the property data pointer of a property.
 * The property must be part of a property pair.
 */
#define ECMA_PROPERTY_VALUE_PTR(property_p) \
  ((ecma_property_value_t *) ECMA_PROPERTY_VALUE_DATA_PTR (property_p))

/**
 * Property reference. It contains the value pointer
 * for real, and the value itself for virtual properties.
 */
typedef union
{
  ecma_property_value_t *value_p; /**< property value pointer for real properties */
  ecma_value_t virtual_value; /**< property value for virtual properties */
} ecma_property_ref_t;

/**
 * Extended property reference, which also contains the
 * property descriptor pointer for real properties.
 */
typedef struct
{
  ecma_property_ref_t property_ref; /**< property reference */
  ecma_property_t *property_p; /**< property descriptor pointer for real properties */
} ecma_extended_property_ref_t;

/**
 * Option flags for ecma_op_object_get_property.
 */
typedef enum
{
  ECMA_PROPERTY_GET_NO_OPTIONS = 0, /**< no option flags for ecma_op_object_get_property */
  ECMA_PROPERTY_GET_VALUE = 1u << 0, /**< fill virtual_value field for virtual properties */
  ECMA_PROPERTY_GET_EXT_REFERENCE = 1u << 1, /**< get extended reference to the property */
} ecma_property_get_option_bits_t;

/**
 * Internal object types.
 */
typedef enum
{
  ECMA_OBJECT_TYPE_GENERAL = 0, /**< all objects that are not belongs to the sub-types below. */
  ECMA_OBJECT_TYPE_CLASS = 1, /**< Objects with class property */
  ECMA_OBJECT_TYPE_ARRAY = 2, /**< Array object (15.4) */
  ECMA_OBJECT_TYPE_PSEUDO_ARRAY  = 3, /**< Array-like object, such as Arguments object (10.6) */
  /* Note: these 4 types must be in this order. See IsCallable operation.  */
  ECMA_OBJECT_TYPE_FUNCTION = 4, /**< Function objects (15.3), created through 13.2 routine */
#if ENABLED (JERRY_ES2015)
  ECMA_OBJECT_TYPE_ARROW_FUNCTION = 5, /**< arrow function objects */
#endif /* ENABLED (JERRY_ES2015) */
  ECMA_OBJECT_TYPE_BOUND_FUNCTION = 6, /**< Function objects (15.3), created through 15.3.4.5 routine */
  ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION = 7, /**< External (host) function object */
  /* Types between 13-15 cannot have a built-in flag. See ecma_lexical_environment_type_t. */

  ECMA_OBJECT_TYPE__MAX /**< maximum value */
} ecma_object_type_t;

/**
 * Types of objects with class property.
 */
typedef enum
{
  ECMA_PSEUDO_ARRAY_ARGUMENTS = 0, /**< Arguments object (10.6) */
  ECMA_PSEUDO_ARRAY_TYPEDARRAY = 1, /**< TypedArray which does NOT need extra space to store length and offset */
  ECMA_PSEUDO_ARRAY_TYPEDARRAY_WITH_INFO = 2, /**< TypedArray which NEEDS extra space to store length and offset */
  ECMA_PSEUDO_ARRAY_ITERATOR = 3, /**< Array iterator object (ECMAScript v6, 22.1.5.1) */
  ECMA_PSEUDO_SET_ITERATOR = 4, /**< Set iterator object (ECMAScript v6, 23.2.5.1) */
  ECMA_PSEUDO_MAP_ITERATOR = 5, /**< Map iterator object (ECMAScript v6, 23.1.5.1) */
  ECMA_PSEUDO_STRING_ITERATOR = 6, /**< String iterator object (ECMAScript v6, 22.1.5.1) */
  ECMA_PSEUDO_SPREAD_OBJECT = 7, /**< spread object */
  ECMA_PSEUDO_ARRAY__MAX = ECMA_PSEUDO_SPREAD_OBJECT /**< maximum value */
} ecma_pseudo_array_type_t;

/**
 * Types of lexical environments.
 */
typedef enum
{
  /* Types between 0 - 12 are ecma_object_type_t which can have a built-in flag. */

  ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE = 13, /**< declarative lexical environment */
  ECMA_LEXICAL_ENVIRONMENT_THIS_OBJECT_BOUND = 14, /**< object-bound lexical environment
                                                    *   with provideThis flag */
  ECMA_LEXICAL_ENVIRONMENT_SUPER_OBJECT_BOUND = 15, /**< object-bound lexical environment
                                                     *   with provided super reference */

  ECMA_LEXICAL_ENVIRONMENT_TYPE_START = ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE, /**< first lexical
                                                                               *   environment type */
  ECMA_LEXICAL_ENVIRONMENT_TYPE__MAX = ECMA_LEXICAL_ENVIRONMENT_SUPER_OBJECT_BOUND /**< maximum value */
} ecma_lexical_environment_type_t;

#if ENABLED (JERRY_ES2015_BUILTIN_ITERATOR)
/**
 * Types of array iterators.
 */
typedef enum
{
  ECMA_ITERATOR_KEYS, /**< List only key indices */
  ECMA_ITERATOR_VALUES, /**< List only key values */
  ECMA_ITERATOR_KEYS_VALUES, /**< List key indices and values */
} ecma_iterator_type_t;
#endif /* ENABLED (JERRY_ES2015_BUILTIN_ITERATOR) */

/**
 * Offset for JERRY_CONTEXT (status_flags) top 8 bits.
 */
#define ECMA_SUPER_EVAL_OPTS_OFFSET (32 - 8)

/**
 * Set JERRY_CONTEXT (status_flags) top 8 bits to the specified 'opts'.
 */
#define ECMA_SET_SUPER_EVAL_PARSER_OPTS(opts) \
  do \
  { \
    JERRY_CONTEXT (status_flags) |= ((uint32_t) opts << ECMA_SUPER_EVAL_OPTS_OFFSET) | ECMA_STATUS_DIRECT_EVAL; \
  } while (0)

/**
 * Get JERRY_CONTEXT (status_flags) top 8 bits.
 */
#define ECMA_GET_SUPER_EVAL_PARSER_OPTS() (JERRY_CONTEXT (status_flags) >> ECMA_SUPER_EVAL_OPTS_OFFSET)

/**
 * Clear JERRY_CONTEXT (status_flags) top 8 bits.
 */
#define ECMA_CLEAR_SUPER_EVAL_PARSER_OPTS() \
  do \
  { \
    JERRY_CONTEXT (status_flags) &= ((1 << ECMA_SUPER_EVAL_OPTS_OFFSET) - 1); \
  } while (0)

/**
 * Ecma object type mask for getting the object type.
 */
#define ECMA_OBJECT_TYPE_MASK 0x0fu

/**
 * Ecma object is built-in or lexical environment. When this flag is set, the object is a
 *   - built-in, if object type is less than ECMA_LEXICAL_ENVIRONMENT_TYPES_START
 *   - lexical environment, if object type is greater or equal than ECMA_LEXICAL_ENVIRONMENT_TYPES_START
 */
#define ECMA_OBJECT_FLAG_BUILT_IN_OR_LEXICAL_ENV 0x10

/**
 * Extensible object.
 */
#define ECMA_OBJECT_FLAG_EXTENSIBLE 0x20

/**
 * Non closure flag for debugger.
 */
#define ECMA_OBJECT_FLAG_BLOCK ECMA_OBJECT_FLAG_EXTENSIBLE

/**
 * Bitshift index for an ecma-object reference count field
 */
#define ECMA_OBJECT_REF_SHIFT 6

/**
 * Bitmask for an ecma-object reference count field
 */
#define ECMA_OBJECT_REF_MASK (((1u << 10) - 1) << ECMA_OBJECT_REF_SHIFT)

/**
 * Value for increasing or decreasing the object reference counter.
 */
#define ECMA_OBJECT_REF_ONE (1u << ECMA_OBJECT_REF_SHIFT)

/**
 * Represents non-visited white object
 */
#define ECMA_OBJECT_NON_VISITED (0x3ffu << ECMA_OBJECT_REF_SHIFT)

/**
 * Maximum value of the object reference counter (1022).
 */
#define ECMA_OBJECT_MAX_REF (ECMA_OBJECT_NON_VISITED - ECMA_OBJECT_REF_ONE)

/**
 * Description of ECMA-object or lexical environment
 * (depending on is_lexical_environment).
 */
typedef struct
{
  /** type : 4 bit : ecma_object_type_t or ecma_lexical_environment_type_t
                     depending on ECMA_OBJECT_FLAG_BUILT_IN_OR_LEXICAL_ENV
      flags : 2 bit : ECMA_OBJECT_FLAG_BUILT_IN_OR_LEXICAL_ENV,
                      ECMA_OBJECT_FLAG_EXTENSIBLE or ECMA_OBJECT_FLAG_BLOCK
      refs : 10 bit (max 1022) */
  uint16_t type_flags_refs;

  /** next in the object chain maintained by the garbage collector */
  jmem_cpointer_t gc_next_cp;

  /** compressed pointer to property list or bound object */
  union
  {
    jmem_cpointer_t property_list_cp; /**< compressed pointer to object's
                                       *  or declerative lexical environments's property list */
    jmem_cpointer_t bound_object_cp;  /**< compressed pointer to lexical environments's the bound object */
  } u1;

  /** object prototype or outer reference */
  union
  {
    jmem_cpointer_t prototype_cp; /**< compressed pointer to the object's prototype  */
    jmem_cpointer_t outer_reference_cp; /**< compressed pointer to the lexical environments's outer reference  */
  } u2;
} ecma_object_t;

/**
 * Description of built-in properties of an object.
 */
typedef struct
{
  uint8_t id; /**< built-in id */
  uint8_t length_and_bitset_size; /**< length for built-in functions and
                                   *   bit set size for all built-ins */
  uint16_t routine_id; /**< routine id for built-in functions */
  uint32_t instantiated_bitset[1]; /**< bit set for instantiated properties */
} ecma_built_in_props_t;

/**
 * Start position of bit set size in length_and_bitset_size field.
 */
#define ECMA_BUILT_IN_BITSET_SHIFT 5

/**
 * Description of extended ECMA-object.
 *
 * The extended object is an object with extra fields.
 */
typedef struct
{
  ecma_object_t object; /**< object header */

  /**
   * Description of extra fields. These extra fields depend on the object type.
   */
  union
  {
    ecma_built_in_props_t built_in; /**< built-in object part */

    /**
     * Description of objects with class.
     */
    struct
    {
      uint16_t class_id; /**< class id of the object */
      uint16_t extra_info; /**< extra information for the object
                            *   e.g. array buffer type info (external/internal) */

      /**
       * Description of extra fields. These extra fields depend on the class_id.
       */
      union
      {
        ecma_value_t value; /**< value of the object (e.g. boolean, number, string, etc.) */
        uint32_t length; /**< length related property (e.g. length of ArrayBuffer) */
      } u;
    } class_prop;

    /**
     * Description of function objects.
     */
    struct
    {
      ecma_value_t scope_cp; /**< function scope */
      ecma_value_t bytecode_cp; /**< function byte code */
    } function;

    /**
     * Description of array objects.
     */
    struct
    {
      uint32_t length; /**< length property value */
      union
      {
        ecma_property_t length_prop; /**< length property */
        uint32_t hole_count; /**< number of array holes in a fast access mode array
                              *   multiplied ECMA_FAST_ACCESS_HOLE_ONE */
      } u;

    } array;

    /**
     * Description of pseudo array objects.
     */
    struct
    {
      uint8_t type; /**< pseudo array type, e.g. Arguments, TypedArray, ArrayIterator */
      uint8_t extra_info; /**< extra information about the object.
                           *   e.g. the specific builtin id for typed arrays,
                           *        [[IterationKind]] property for %Iterator% */
      union
      {
        uint16_t length; /**< for arguments: length of names */
        uint16_t class_id; /**< for typedarray: the specific class name id */
        uint16_t iterator_index; /**< for %Iterator%: [[%Iterator%NextIndex]] property */
      } u1;
      union
      {
        ecma_value_t lex_env_cp; /**< for arguments: lexical environment */
        ecma_value_t arraybuffer; /**< for typedarray: internal arraybuffer */
        ecma_value_t iterated_value; /**< for %Iterator%: [[IteratedObject]] property */
        ecma_value_t spread_value; /**< for spread object: spreaded element */
      } u2;
    } pseudo_array;

    /**
     * Description of bound function object.
     */
    struct
    {
      ecma_value_t target_function; /**< target function */
      ecma_value_t args_len_or_this; /**< length of arguments or this value */
    } bound_function;

    ecma_external_handler_t external_handler_cb; /**< external function */
  } u;
} ecma_extended_object_t;

/**
 * Description of built-in extended ECMA-object.
 */
typedef struct
{
  ecma_extended_object_t extended_object; /**< extended object part */
  ecma_built_in_props_t built_in; /**< built-in object part */
} ecma_extended_built_in_object_t;

/**
 * Alignment for the fast access mode array length.
 * The real length is aligned up for allocating the underlying buffer.
 */
#define ECMA_FAST_ARRAY_ALIGNMENT (8)

/**
 * Align the length of the fast mode array to get the allocated size of the underlying buffer
 */
#define ECMA_FAST_ARRAY_ALIGN_LENGTH(length) \
  (uint32_t) ((((length)) + ECMA_FAST_ARRAY_ALIGNMENT - 1) / ECMA_FAST_ARRAY_ALIGNMENT * ECMA_FAST_ARRAY_ALIGNMENT)

/**
 * Compiled byte code data.
 */
typedef struct
{
  uint16_t size;                    /**< real size >> JMEM_ALIGNMENT_LOG */
  uint16_t refs;                    /**< reference counter for the byte code */
  uint16_t status_flags;            /**< various status flags:
                                     *   CBC_CODE_FLAGS_FUNCTION flag tells whether
                                     *   the byte code is function or regular expression.
                                     *   If function, the other flags must be CBC_CODE_FLAGS...
                                     *   If regexp, the other flags must be RE_FLAG... */
} ecma_compiled_code_t;

#if ENABLED (JERRY_SNAPSHOT_EXEC)

/**
 * Description of static function objects.
 */
typedef struct
{
  ecma_extended_object_t header; /**< header part */
  const ecma_compiled_code_t *bytecode_p; /**< real byte code pointer */
} ecma_static_function_t;

#endif /* ENABLED (JERRY_SNAPSHOT_EXEC) */

#if ENABLED (JERRY_ES2015)

/**
 * Description of arrow function objects.
 */
typedef struct
{
  ecma_object_t object; /**< object header */
  ecma_value_t this_binding; /**< value of 'this' binding */
  jmem_cpointer_t scope_cp; /**< function scope */
  jmem_cpointer_t bytecode_cp; /**< function byte code */
} ecma_arrow_function_t;

#if ENABLED (JERRY_SNAPSHOT_EXEC)

/**
 * Description of static arrow function objects.
 */
typedef struct
{
  ecma_arrow_function_t header;
  const ecma_compiled_code_t *bytecode_p;
} ecma_static_arrow_function_t;

#endif /* ENABLED (JERRY_SNAPSHOT_EXEC) */

#endif /* ENABLED (JERRY_ES2015) */

#if ENABLED (JERRY_ES2015_BUILTIN_MAP) || ENABLED (JERRY_ES2015_BUILTIN_SET)
/**
 * Description of Map/Set objects.
 */
typedef struct
{
  ecma_extended_object_t header; /**< header part */
  uint32_t size; /**< size of the map object */
} ecma_map_object_t;
#endif /* ENABLED (JERRY_ES2015_BUILTIN_MAP) || ENABLED (JERRY_ES2015_BUILTIN_SET) */

typedef enum
{
  ECMA_PROP_NO_OPTS = (0), /** empty property descriptor */
  ECMA_PROP_IS_GET_DEFINED = (1 << 0), /** Is [[Get]] defined? */
  ECMA_PROP_IS_SET_DEFINED = (1 << 1), /** Is [[Set]] defined? */

  ECMA_PROP_IS_CONFIGURABLE = (1 << 2), /** [[Configurable]] */
  ECMA_PROP_IS_ENUMERABLE = (1 << 3), /** [[Enumerable]] */
  ECMA_PROP_IS_WRITABLE = (1 << 4), /** [[Writable]] */
  ECMA_PROP_IS_THROW = (1 << 5), /** Flag that controls failure handling */

  ECMA_PROP_IS_VALUE_DEFINED = (1 << 6), /** Is [[Value]] defined? */
  ECMA_PROP_IS_CONFIGURABLE_DEFINED = (1 << 7), /** Is [[Configurable]] defined? */
  ECMA_PROP_IS_ENUMERABLE_DEFINED = (1 << 8), /** Is [[Enumerable]] defined? */
  ECMA_PROP_IS_WRITABLE_DEFINED = (1 << 9), /** Is [[Writable]] defined? */
} ecma_property_descriptor_status_flags_t;

/**
 * Description of ECMA property descriptor
 *
 * See also: ECMA-262 v5, 8.10.
 *
 * Note:
 *      If a component of descriptor is undefined then corresponding
 *      field should contain it's default value.
 *      The struct members must be in this order or keep in sync with ecma_property_flags_t and ECMA_IS_THROW flag.
 */
typedef struct
{

  /** any combination of ecma_property_descriptor_status_flags_t bits */
  uint16_t flags;

  /** [[Value]] */
  ecma_value_t value;

  /** [[Get]] */
  ecma_object_t *get_p;

  /** [[Set]] */
  ecma_object_t *set_p;
} ecma_property_descriptor_t;

/**
 * Bitfield which represents a namedata property options in an ecma_property_descriptor_t
 * Attributes:
 *  - is_get_defined, is_set_defined : false
 *  - is_configurable, is_writable, is_enumerable : undefined (false)
 *  - is_throw : undefined (false)
 *  - is_value_defined : true
 *  - is_configurable_defined, is_writable_defined, is_enumerable_defined : true
 */
#define ECMA_NAME_DATA_PROPERTY_DESCRIPTOR_BITS 0x3c0

/**
 * Bitmask to get a the physical property flags from an ecma_property_descriptor
 */
#define ECMA_PROPERTY_FLAGS_MASK 0x1c

/**
 * Flag that controls failure handling during defining property
 *
 * Note: This flags represents the [[DefineOwnProperty]] (P, Desc, Throw) 3rd argument
 */
#define ECMA_IS_THROW (1 << 5)

#if !ENABLED (JERRY_NUMBER_TYPE_FLOAT64)
/**
 * Description of an ecma-number
 */
typedef float ecma_number_t;

/**
 * It makes possible to read/write an ecma_number_t as uint32_t without strict aliasing rule violation.
 */
typedef union
{
  ecma_number_t as_ecma_number_t;
  uint32_t as_uint32_t;
} ecma_number_accessor_t;

#define DOUBLE_TO_ECMA_NUMBER_T(value) (ecma_number_t) (value)

/**
 * Maximum number of significant digits that ecma-number can store
 */
#define ECMA_NUMBER_MAX_DIGITS  (9)

/**
 * Width of sign field
 *
 * See also:
 *          IEEE-754 2008, 3.6, Table 3.5
 */
#define ECMA_NUMBER_SIGN_WIDTH       (1)

/**
 * Width of biased exponent field
 *
 * See also:
 *          IEEE-754 2008, 3.6, Table 3.5
 */
#define ECMA_NUMBER_BIASED_EXP_WIDTH (8)

/**
 * Width of fraction field
 *
 * See also:
 *          IEEE-754 2008, 3.6, Table 3.5
 */
#define ECMA_NUMBER_FRACTION_WIDTH   (23)
#elif ENABLED (JERRY_NUMBER_TYPE_FLOAT64)
/**
 * Description of an ecma-number
 */
typedef double ecma_number_t;

/**
 * It makes possible to read/write an ecma_number_t as uint64_t without strict aliasing rule violation.
 */
typedef union
{
  ecma_number_t as_ecma_number_t;
  uint64_t as_uint64_t;
} ecma_number_accessor_t;

#define DOUBLE_TO_ECMA_NUMBER_T(value) value

/**
 * Maximum number of significant digits that ecma-number can store
 */
#define ECMA_NUMBER_MAX_DIGITS  (19)

/**
 * Width of sign field
 *
 * See also:
 *          IEEE-754 2008, 3.6, Table 3.5
 */
#define ECMA_NUMBER_SIGN_WIDTH       (1)

/**
 * Width of biased exponent field
 *
 * See also:
 *          IEEE-754 2008, 3.6, Table 3.5
 */
#define ECMA_NUMBER_BIASED_EXP_WIDTH (11)

/**
 * Width of fraction field
 *
 * See also:
 *          IEEE-754 2008, 3.6, Table 3.5
 */
#define ECMA_NUMBER_FRACTION_WIDTH   (52)
#endif /* !ENABLED (JERRY_NUMBER_TYPE_FLOAT64) */

/**
 * Value '0' of ecma_number_t
 */
#define ECMA_NUMBER_ZERO ((ecma_number_t) 0)

/**
 * Value '1' of ecma_number_t
 */
#define ECMA_NUMBER_ONE  ((ecma_number_t) 1)

/**
 * Value '2' of ecma_number_t
 */
#define ECMA_NUMBER_TWO  ((ecma_number_t) 2)

/**
 * Value '0.5' of ecma_number_t
 */
#define ECMA_NUMBER_HALF ((ecma_number_t) 0.5f)

/**
 * Value '-1' of ecma_number_t
 */
#define ECMA_NUMBER_MINUS_ONE ((ecma_number_t) -1)

#if !ENABLED (JERRY_NUMBER_TYPE_FLOAT64)
/**
 * Number.MIN_VALUE (i.e., the smallest positive value of ecma-number)
 *
 * See also: ECMA_262 v5, 15.7.3.3
 */
# define ECMA_NUMBER_MIN_VALUE (FLT_MIN)
/**
 * Number.MAX_VALUE (i.e., the maximum value of ecma-number)
 *
 * See also: ECMA_262 v5, 15.7.3.2
 */
# define ECMA_NUMBER_MAX_VALUE (FLT_MAX)
/**
 * Number.EPSILON
 *
 * See also: ECMA_262 v6, 20.1.2.1
 */
# define  ECMA_NUMBER_EPSILON ((ecma_number_t) 1.1920928955078125e-7)

/**
 * Number.MAX_SAFE_INTEGER
 *
 * See also: ECMA_262 v6, 20.1.2.6
 */
# define ECMA_NUMBER_MAX_SAFE_INTEGER ((ecma_number_t) 0xFFFFFF)

/**
 * Number.MIN_SAFE_INTEGER
 *
 * See also: ECMA_262 v6, 20.1.2.8
 */
# define ECMA_NUMBER_MIN_SAFE_INTEGER ((ecma_number_t) -0xFFFFFF)
#elif ENABLED (JERRY_NUMBER_TYPE_FLOAT64)
/**
 * Number.MAX_VALUE (i.e., the maximum value of ecma-number)
 *
 * See also: ECMA_262 v5, 15.7.3.2
 */
# define ECMA_NUMBER_MAX_VALUE ((ecma_number_t) 1.7976931348623157e+308)

/**
 * Number.MIN_VALUE (i.e., the smallest positive value of ecma-number)
 *
 * See also: ECMA_262 v5, 15.7.3.3
 */
# define ECMA_NUMBER_MIN_VALUE ((ecma_number_t) 5e-324)

/**
 * Number.EPSILON
 *
 * See also: ECMA_262 v6, 20.1.2.1
 */
# define  ECMA_NUMBER_EPSILON ((ecma_number_t) 2.2204460492503130808472633361816e-16)

/**
 * Number.MAX_SAFE_INTEGER
 *
 * See also: ECMA_262 v6, 20.1.2.6
 */
# define ECMA_NUMBER_MAX_SAFE_INTEGER ((ecma_number_t) 0x1FFFFFFFFFFFFF)

/**
 * Number.MIN_SAFE_INTEGER
 *
 * See also: ECMA_262 v6, 20.1.2.8
 */
# define ECMA_NUMBER_MIN_SAFE_INTEGER ((ecma_number_t) -0x1FFFFFFFFFFFFF)
#endif /* !ENABLED (JERRY_NUMBER_TYPE_FLOAT64) */

/**
 * Euler number
 */
#define ECMA_NUMBER_E  ((ecma_number_t) 2.7182818284590452354)

/**
 * Natural logarithm of 10
 */
#define ECMA_NUMBER_LN10 ((ecma_number_t) 2.302585092994046)

/**
 * Natural logarithm of 2
 */
#define ECMA_NUMBER_LN2 ((ecma_number_t) 0.6931471805599453)

/**
 * Logarithm base 2 of the Euler number
 */
#define ECMA_NUMBER_LOG2E ((ecma_number_t) 1.4426950408889634)

/**
 * Logarithm base 10 of the Euler number
 */
#define ECMA_NUMBER_LOG10E ((ecma_number_t) 0.4342944819032518)

/**
 * Pi number
 */
#define ECMA_NUMBER_PI  ((ecma_number_t) 3.1415926535897932)

/**
 * Square root of 0.5
 */
#define ECMA_NUMBER_SQRT_1_2  ((ecma_number_t) 0.7071067811865476)

/**
 * Square root of 2
 */
#define ECMA_NUMBER_SQRT2  ((ecma_number_t) 1.4142135623730951)

/**
 * Maximum number of characters in string representation of ecma-number
 */
#define ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER 64

/**
 * Maximum number of characters in string representation of ecma-uint32
 */
#define ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32 10

/**
 * String is not a valid array index.
 */
#define ECMA_STRING_NOT_ARRAY_INDEX UINT32_MAX

/**
 * Ecma-collection: a growable list of ecma-values.
 */
typedef struct
{
  uint32_t item_count; /**< number of items in the collection */
  uint32_t capacity; /**< number of items can be stored in the underlying buffer */
  ecma_value_t *buffer_p; /**< underlying data buffer */
} ecma_collection_t;

/**
 * Initial capacity of an ecma-collection
 */
#define ECMA_COLLECTION_INITIAL_CAPACITY 4

/**
 * Ecma-collenction grow factor when the collection underlying buffer need to be reallocated
 */
#define ECMA_COLLECTION_GROW_FACTOR (ECMA_COLLECTION_INITIAL_CAPACITY * 2)

/**
 * Compute the total allocated size of the collection based on it's capacity
 */
#define ECMA_COLLECTION_ALLOCATED_SIZE(capacity) \
  (uint32_t) (sizeof (ecma_collection_t) + (capacity * sizeof (ecma_value_t)))

/**
 * Initial allocated size of an ecma-collection
 */
#define ECMA_COLLECTION_INITIAL_SIZE ECMA_COLLECTION_ALLOCATED_SIZE (ECMA_COLLECTION_INITIAL_CAPACITY)

/**
 * Direct string types (2 bit).
 */
typedef enum
{
  ECMA_DIRECT_STRING_PTR = 0, /**< string is a string pointer, only used by property names */
  ECMA_DIRECT_STRING_MAGIC = 1, /**< string is a magic string */
  ECMA_DIRECT_STRING_UINT = 2, /**< string is an unsigned int */
  ECMA_DIRECT_STRING_ECMA_INTEGER = 3, /**< string is an ecma-integer */
} ecma_direct_string_type_t;

/**
 * Maximum value of the immediate part of a direct magic string.
 * Must be compatible with the immediate property name.
 */
#if ENABLED (JERRY_CPOINTER_32_BIT)
#define ECMA_DIRECT_STRING_MAX_IMM 0x07ffffff
#else /* !ENABLED (JERRY_CPOINTER_32_BIT) */
#define ECMA_DIRECT_STRING_MAX_IMM 0x0000ffff
#endif /* ENABLED (JERRY_CPOINTER_32_BIT) */

/**
 * Shift for direct string value part in ecma_value_t.
 */
#define ECMA_DIRECT_STRING_SHIFT (ECMA_VALUE_SHIFT + 2)

/**
 * Full mask for direct strings.
 */
#define ECMA_DIRECT_STRING_MASK ((uintptr_t) (ECMA_DIRECT_TYPE_MASK | (0x3u << ECMA_VALUE_SHIFT)))

/**
 * Create an ecma direct string.
 */
#define ECMA_CREATE_DIRECT_STRING(type, value) \
  ((uintptr_t) (ECMA_TYPE_DIRECT_STRING | ((type) << ECMA_VALUE_SHIFT) | (value) << ECMA_DIRECT_STRING_SHIFT))

/**
 * Create an ecma direct string from the given number.
 *
 * Note: the given number must be less or equal than ECMA_DIRECT_STRING_MAX_IMM
 */
#define ECMA_CREATE_DIRECT_UINT32_STRING(uint32_number) \
  ((ecma_string_t *) ECMA_CREATE_DIRECT_STRING (ECMA_DIRECT_STRING_UINT, (uintptr_t) uint32_number))

/**
 * Checks whether the string is direct.
 */
#define ECMA_IS_DIRECT_STRING(string_p) \
  ((((uintptr_t) (string_p)) & 0x1) != 0)

/**
 * Checks whether the string is direct.
 */
#define ECMA_IS_DIRECT_STRING_WITH_TYPE(string_p, type) \
  ((((uintptr_t) (string_p)) & ECMA_DIRECT_STRING_MASK) == ECMA_CREATE_DIRECT_STRING (type, 0))

/**
 * Returns the type of a direct string.
 */
#define ECMA_GET_DIRECT_STRING_TYPE(string_p) \
  ((((uintptr_t) (string_p)) >> ECMA_VALUE_SHIFT) & 0x3)

/**
 * Shift applied to type conversions.
 */
#define ECMA_STRING_TYPE_CONVERSION_SHIFT (ECMA_PROPERTY_NAME_TYPE_SHIFT - ECMA_VALUE_SHIFT)

/**
 * Converts direct string type to property name type.
 */
#define ECMA_DIRECT_STRING_TYPE_TO_PROP_NAME_TYPE(string_p) \
  ((((uintptr_t) (string_p)) & (0x3 << ECMA_VALUE_SHIFT)) << ECMA_STRING_TYPE_CONVERSION_SHIFT)

/**
 * Returns the value of a direct string.
 */
#define ECMA_GET_DIRECT_STRING_VALUE(string_p) \
  (((uintptr_t) (string_p)) >> ECMA_DIRECT_STRING_SHIFT)

/**
 * Maximum number of bytes that a long-utf8-string is able to store
 */
#define ECMA_STRING_SIZE_LIMIT UINT32_MAX

typedef enum
{
  ECMA_STRING_CONTAINER_HEAP_UTF8_STRING, /**< actual data is on the heap as an utf-8 (cesu8) string
                                           *   maximum size is 2^16. */
  ECMA_STRING_CONTAINER_HEAP_LONG_UTF8_STRING, /**< actual data is on the heap as an utf-8 (cesu8) string
                                                *   maximum size is 2^32. */
  ECMA_STRING_CONTAINER_UINT32_IN_DESC, /**< actual data is UInt32-represeneted Number
                                             stored locally in the string's descriptor */
  ECMA_STRING_CONTAINER_HEAP_ASCII_STRING, /**< actual data is on the heap as an ASCII string
                                            *   maximum size is 2^16. */
  ECMA_STRING_CONTAINER_MAGIC_STRING_EX, /**< the ecma-string is equal to one of external magic strings */

  ECMA_STRING_CONTAINER_SYMBOL, /**< the ecma-string is a symbol */

  ECMA_STRING_CONTAINER_MAP_KEY, /**< the ecma-string is a map key string */

  ECMA_STRING_CONTAINER__MAX = ECMA_STRING_CONTAINER_MAP_KEY /**< maximum value */
} ecma_string_container_t;

/**
 * Mask for getting the container of a string.
 */
#define ECMA_STRING_CONTAINER_MASK 0x7u

/**
 * Value for increasing or decreasing the reference counter.
 */
#define ECMA_STRING_REF_ONE (1u << 4)

/**
 * Maximum value of the reference counter (4294967280).
 */
#define ECMA_STRING_MAX_REF (0xFFFFFFF0)

/**
 * Flag that identifies that the string is static which means it is stored in JERRY_CONTEXT (string_list_cp)
 */
#define ECMA_STATIC_STRING_FLAG (1 << 3)

/**
 * Set an ecma-string as static string
 */
#define ECMA_SET_STRING_AS_STATIC(string_p) \
  (string_p)->refs_and_container |= ECMA_STATIC_STRING_FLAG

/**
 * Checks whether the ecma-string is static string
 */
#define ECMA_STRING_IS_STATIC(string_p) \
  ((string_p)->refs_and_container & ECMA_STATIC_STRING_FLAG)

/**
 * Returns with the container type of a string.
 */
#define ECMA_STRING_GET_CONTAINER(string_desc_p) \
  ((ecma_string_container_t) ((string_desc_p)->refs_and_container & ECMA_STRING_CONTAINER_MASK))

/**
 * Checks whether the reference counter is 1.
 */
#define ECMA_STRING_IS_REF_EQUALS_TO_ONE(string_desc_p) \
  (((string_desc_p)->refs_and_container >> 4) == 1)

/**
 * ECMA string-value descriptor
 */
typedef struct
{
  /** Reference counter for the string */
  uint32_t refs_and_container;

  /**
   * Actual data or identifier of it's place in container (depending on 'container' field)
   */
  union
  {
    lit_string_hash_t hash; /**< hash of the ASCII/UTF8 string */
    uint32_t magic_string_ex_id; /**< identifier of an external magic string (lit_magic_string_ex_id_t) */
    uint32_t uint32_number; /**< uint32-represented number placed locally in the descriptor */
  } u;
} ecma_string_t;

/**
 * ECMA ASCII string-value descriptor
 */
typedef struct
{
  ecma_string_t header; /**< string header */
  uint16_t size; /**< size of this ASCII string in bytes */
} ecma_ascii_string_t;

/**
 * ECMA long UTF8 string-value descriptor
 */
typedef struct
{
  ecma_string_t header; /**< string header */
  uint16_t size; /**< size of this utf-8 string in bytes */
  uint16_t length; /**< length of this utf-8 string in bytes */
} ecma_utf8_string_t;

/**
 * ECMA UTF8 string-value descriptor
 */
typedef struct
{
  ecma_string_t header; /**< string header */
  lit_utf8_size_t size; /**< size of this long utf-8 string in bytes */
  lit_utf8_size_t length; /**< length of this long utf-8 string in bytes */
} ecma_long_utf8_string_t;

/**
 * Get the start position of the string buffer of an ecma ASCII string
 */
#define ECMA_ASCII_STRING_GET_BUFFER(string_p) \
  ((lit_utf8_byte_t *) ((lit_utf8_byte_t *) (string_p) +  sizeof (ecma_ascii_string_t)))

/**
 * Get the start position of the string buffer of an ecma UTF8 string
 */
#define ECMA_UTF8_STRING_GET_BUFFER(string_p) \
  ((lit_utf8_byte_t *) ((lit_utf8_byte_t *) (string_p) +  sizeof (ecma_utf8_string_t)))

/**
 * Get the start position of the string buffer of an ecma long UTF8 string
 */
#define ECMA_LONG_UTF8_STRING_GET_BUFFER(string_p) \
  ((lit_utf8_byte_t *) ((lit_utf8_byte_t *) (string_p) +  sizeof (ecma_long_utf8_string_t)))

/**
 * ECMA extended string-value descriptor
 */
typedef struct
{
  ecma_string_t header; /**< string header */

  union
  {
    ecma_value_t symbol_descriptor; /**< symbol descriptor string-value */
    ecma_value_t value; /**< original key value corresponds to the map key string */
  } u;
} ecma_extended_string_t;

/**
 * String builder header
 */
typedef struct
{
  lit_utf8_size_t current_size; /**< size of the data in the buffer */
} ecma_stringbuilder_header_t;

/**
 * Get pointer to the beginning of the stored string in the string builder
 */
#define ECMA_STRINGBUILDER_STRING_PTR(header_p) \
  ((lit_utf8_byte_t *) (((lit_utf8_byte_t *) header_p) + sizeof (ecma_ascii_string_t)))

/**
 * Get the size of the stored string in the string builder
 */
#define ECMA_STRINGBUILDER_STRING_SIZE(header_p) \
  ((lit_utf8_size_t) (header_p->current_size - sizeof (ecma_ascii_string_t)))

/**
 * String builder handle
 */
typedef struct
{
  ecma_stringbuilder_header_t *header_p; /**< pointer to header */
} ecma_stringbuilder_t;

/**
 * Abort flag for error reference.
 */
#define ECMA_ERROR_REF_ABORT 0x1

/**
 * Value for increasing or decreasing the reference counter.
 */
#define ECMA_ERROR_REF_ONE (1u << 1)

/**
 * Maximum value of the reference counter.
 */
#define ECMA_ERROR_MAX_REF (UINT32_MAX - 1u)

/**
 * Representation of a thrown value on API level.
 */
typedef struct
{
  uint32_t refs_and_flags; /**< reference counter */
  ecma_value_t value; /**< referenced value */
} ecma_error_reference_t;

#if ENABLED (JERRY_PROPRETY_HASHMAP)

/**
 * The lowest state of the ecma_prop_hashmap_alloc_state counter.
 * If ecma_prop_hashmap_alloc_state other other than this value, it is
 * disabled.
 */
#define ECMA_PROP_HASHMAP_ALLOC_ON 0

/**
 * The highest state of the ecma_prop_hashmap_alloc_state counter.
 */
#define ECMA_PROP_HASHMAP_ALLOC_MAX 4

#endif /* ENABLED (JERRY_PROPRETY_HASHMAP) */

/**
 * Number of values in a literal storage item
 */
#define ECMA_LIT_STORAGE_VALUE_COUNT 3

/**
 * Literal storage item
 */
typedef struct
{
  jmem_cpointer_t next_cp; /**< cpointer ot next item */
  jmem_cpointer_t values[ECMA_LIT_STORAGE_VALUE_COUNT]; /**< list of values */
} ecma_lit_storage_item_t;

/**
 * Number storage item
 */
typedef struct
{
  jmem_cpointer_t next_cp; /**< cpointer ot next item */
  jmem_cpointer_t values[ECMA_LIT_STORAGE_VALUE_COUNT]; /**< list of values */
} ecma_number_storage_item_t;

#if ENABLED (JERRY_LCACHE)
/**
 * Container of an LCache entry identifier
 */
#if ENABLED (JERRY_CPOINTER_32_BIT)
typedef uint64_t ecma_lcache_hash_entry_id_t;
#else /* !ENABLED (JERRY_CPOINTER_32_BIT) */
typedef uint32_t ecma_lcache_hash_entry_id_t;
#endif /* ENABLED (JERRY_CPOINTER_32_BIT) */

/**
 * Entry of LCache hash table
 */
typedef struct
{
  /** Pointer to a property of the object */
  ecma_property_t *prop_p;

  /** Entry identifier in LCache */
  ecma_lcache_hash_entry_id_t id;
} ecma_lcache_hash_entry_t;

/**
 * Number of rows in LCache's hash table
 */
#define ECMA_LCACHE_HASH_ROWS_COUNT 128

/**
 * Number of entries in a row of LCache's hash table
 */
#define ECMA_LCACHE_HASH_ROW_LENGTH 2

#endif /* ENABLED (JERRY_LCACHE) */

#if ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY)

/**
 * Function callback descriptor of a %TypedArray% object getter
 */
typedef ecma_number_t (*ecma_typedarray_getter_fn_t)(lit_utf8_byte_t *src);

/**
 * Function callback descriptor of a %TypedArray% object setter
 */
typedef void (*ecma_typedarray_setter_fn_t)(lit_utf8_byte_t *src, ecma_number_t value);

/**
 * Builtin id for the different types of TypedArray's
 */
typedef enum
{
  ECMA_INT8_ARRAY,          /**< Int8Array */
  ECMA_UINT8_ARRAY,         /**< Uint8Array */
  ECMA_UINT8_CLAMPED_ARRAY, /**< Uint8ClampedArray */
  ECMA_INT16_ARRAY,         /**< Int16Array */
  ECMA_UINT16_ARRAY,        /**< Uint16Array */
  ECMA_INT32_ARRAY,         /**< Int32Array */
  ECMA_UINT32_ARRAY,        /**< Uint32Array */
  ECMA_FLOAT32_ARRAY,       /**< Float32Array */
  ECMA_FLOAT64_ARRAY,       /**< Float64Array */
} ecma_typedarray_type_t;

/**
 * Extra information for ArrayBuffers.
 */
typedef enum
{
  ECMA_ARRAYBUFFER_INTERNAL_MEMORY = 0u,        /* ArrayBuffer memory is handled internally. */
  ECMA_ARRAYBUFFER_EXTERNAL_MEMORY = (1u << 0), /* ArrayBuffer created via jerry_create_arraybuffer_external. */
} ecma_arraybuffer_extra_flag_t;

#define ECMA_ARRAYBUFFER_HAS_EXTERNAL_MEMORY(object_p) \
    ((((ecma_extended_object_t *) object_p)->u.class_prop.extra_info & ECMA_ARRAYBUFFER_EXTERNAL_MEMORY) != 0)

/**
 * Struct to store information for ArrayBuffers with external memory.
 *
 * The following elements are stored in Jerry memory.
 *
 *  buffer_p - pointer to the external memory.
 *  free_cb - pointer to a callback function which is called when the ArrayBuffer is freed.
 */
typedef struct
{
  ecma_extended_object_t extended_object; /**< extended object part */
  void *buffer_p; /**< external buffer pointer */
  ecma_object_native_free_callback_t free_cb; /**<  the free callback for the above buffer pointer */
} ecma_arraybuffer_external_info;

/**
 * Some internal properties of TypedArray object.
 * It is only used when the offset is not 0, and
 * the array-length is not buffer-length / element_size.
 */
typedef struct
{
  ecma_extended_object_t extended_object; /**< extended object part */
  ecma_length_t byte_offset; /**< the byteoffset of the above arraybuffer */
  ecma_length_t array_length; /**< the array length */
} ecma_extended_typedarray_object_t;

/**
 * General structure for query %TypedArray% object's properties.
 **/
typedef struct
{
  ecma_object_t *array_buffer_p; /**< pointer to the typedArray's [[ViewedArrayBuffer]] internal slot */
  lit_utf8_byte_t *buffer_p; /**< pointer to the underlying raw data buffer.
                              *   Note:
                              *    - This address is increased by the [ByteOffset]] internal property.
                              *    - This address must be used during indexed read/write operation. */
  ecma_typedarray_type_t id; /**< [[TypedArrayName]] internal slot */
  uint32_t length; /**< [[ByteLength]] internal slot */
  ecma_length_t offset; /**< [[ByteOffset]] internal slot. */
  uint8_t shift; /**< the element size shift in the typedArray */
  uint8_t element_size; /**< element size based on [[TypedArrayName]] in Table 49 */
} ecma_typedarray_info_t;

#endif /* ENABLED (JERRY_ES2015_BUILTIN_TYPEDARRAY) */

#if ENABLED (JERRY_ES2015_BUILTIN_DATAVIEW)
/**
 * Description of DataView objects.
 */
typedef struct
{
  ecma_extended_object_t header; /**< header part */
  ecma_object_t *buffer_p; /**< [[ViewedArrayBuffer]] internal slot */
  uint32_t byte_offset; /**< [[ByteOffset]] internal slot */
} ecma_dataview_object_t;
#endif /* ENABLED (JERRY_ES2015_BUILTIN_DATAVIEW */

/**
 * Flag for indicating whether the symbol is a well known symbol
 *
 * See also: 6.1.5.1
 */
#define ECMA_GLOBAL_SYMBOL_FLAG 0x01

/**
 * Bitshift index for indicating whether the symbol is a well known symbol
 *
 * See also: 6.1.5.1
 */
#define ECMA_GLOBAL_SYMBOL_SHIFT 1

/**
 * Bitshift index for the symbol hash property
 */
#define ECMA_SYMBOL_HASH_SHIFT 2

#if (JERRY_STACK_LIMIT != 0)
/**
 * Check the current stack usage. If the limit is reached a RangeError is raised.
 */
#define ECMA_CHECK_STACK_USAGE() \
do \
{ \
  if (ecma_get_current_stack_usage () > CONFIG_MEM_STACK_LIMIT) \
  { \
    return ecma_raise_range_error (ECMA_ERR_MSG ("Maximum call stack size exceeded.")); \
  } \
} while (0)
#else /* JERRY_STACK_LIMIT == 0) */
/**
 * If the stack limit is unlimited, this check is an empty macro.
 */
#define ECMA_CHECK_STACK_USAGE()
#endif /* (JERRY_STACK_LIMIT != 0) */

/**
 * @}
 * @}
 */

#endif  /* !ECMA_GLOBALS_H */
