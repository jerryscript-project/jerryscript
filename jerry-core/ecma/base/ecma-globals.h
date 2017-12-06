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
 * Ecma-pointer field is used to calculate ecma value's address.
 *
 * Ecma-pointer contains value's shifted offset from common Ecma-pointers' base.
 * The offset is shifted right by JMEM_ALIGNMENT_LOG.
 * Least significant JMEM_ALIGNMENT_LOG bits of non-shifted offset are zeroes.
 */
#ifdef JERRY_CPOINTER_32_BIT
#define ECMA_POINTER_FIELD_WIDTH 32
#else /* !JERRY_CPOINTER_32_BIT */
#define ECMA_POINTER_FIELD_WIDTH 16
#endif /* JERRY_CPOINTER_32_BIT */

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
 * Type of ecma value
 */
typedef enum
{
  ECMA_TYPE_DIRECT = 0, /**< directly encoded value, a 28 bit signed integer or a simple value */
  ECMA_TYPE_FLOAT = 1, /**< pointer to a 64 or 32 bit floating point number */
  ECMA_TYPE_STRING = 2, /**< pointer to description of a string */
  ECMA_TYPE_OBJECT = 3, /**< pointer to description of an object */
  ECMA_TYPE_ERROR = 7, /**< pointer to description of an error reference */
  ECMA_TYPE___MAX = ECMA_TYPE_ERROR /** highest value for ecma types */
} ecma_type_t;

/**
 * Description of an ecma value
 *
 * Bit-field structure: type (2) | error (1) | value (29)
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
 * Mask for ecma types in ecma_type_t
 */
#define ECMA_VALUE_TYPE_MASK 0x7u

/**
 * Shift for value part in ecma_type_t
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

/* ECMA make simple value */
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
};

/**
 * Maximum integer number for an ecma value
 */
#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32
#define ECMA_INTEGER_NUMBER_MAX         0x7fffff
#define ECMA_INTEGER_NUMBER_MAX_SHIFTED 0x7fffff0
#else /* CONFIG_ECMA_NUMBER_TYPE != CONFIG_ECMA_NUMBER_FLOAT32 */
#define ECMA_INTEGER_NUMBER_MAX         0x7ffffff
#define ECMA_INTEGER_NUMBER_MAX_SHIFTED 0x7ffffff0
#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32 */

/**
 * Minimum integer number for an ecma value
 */
#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32
#define ECMA_INTEGER_NUMBER_MIN         -0x7fffff
#define ECMA_INTEGER_NUMBER_MIN_SHIFTED -0x7fffff0
#else /* CONFIG_ECMA_NUMBER_TYPE != CONFIG_ECMA_NUMBER_FLOAT32 */
#define ECMA_INTEGER_NUMBER_MIN         -0x8000000
#define ECMA_INTEGER_NUMBER_MIN_SHIFTED (-0x7fffffff - 1) /* -0x80000000 */
#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32 */

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
#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32
#define ECMA_INTEGER_MULTIPLY_MAX 0xb50
#else /* CONFIG_ECMA_NUMBER_TYPE != CONFIG_ECMA_NUMBER_FLOAT32 */
#define ECMA_INTEGER_MULTIPLY_MAX 0x2d41
#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32 */

/**
 * Checks whether the error flag is set.
 */
#define ECMA_IS_VALUE_ERROR(value) \
  (unlikely ((value) == ECMA_VALUE_ERROR))

/**
 * Representation for native external pointer
 */
typedef uintptr_t ecma_external_pointer_t;

/**
 * Callback which tells whether the ECMAScript execution should be stopped.
 */
typedef ecma_value_t (*ecma_vm_exec_stop_callback_t) (void *user_p);

/**
 * Function type for user context deallocation
 */
typedef void (*ecma_user_context_deinit_t) (void *user_context_p);

/**
 * Type of an external function handler.
 */
typedef ecma_value_t (*ecma_external_handler_t) (const ecma_value_t function_obj,
                                                 const ecma_value_t this_val,
                                                 const ecma_value_t args_p[],
                                                 const ecma_length_t args_count);

/**
 * Native free callback of an object (deprecated).
 */
typedef void (*ecma_object_free_callback_t) (const uintptr_t native_p);

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
typedef struct
{
  void *data_p; /**< points to the data of the object */
  union
  {
    ecma_object_free_callback_t callback_p; /**< callback */
    ecma_object_native_info_t *info_p; /**< native info */
  } u;
} ecma_native_pointer_t;

/**
 * Special property identifiers.
 */
typedef enum
{
  ECMA_SPECIAL_PROPERTY_DELETED, /**< deleted property */

  /* Note: when new special types are added
   * ECMA_PROPERTY_IS_PROPERTY_PAIR must be updated as well. */
  ECMA_SPECIAL_PROPERTY_HASHMAP, /**< hashmap property */

  ECMA_SPECIAL_PROPERTY__COUNT /**< Number of special property types */
} ecma_internal_property_id_t;

/**
 * Property's 'Writable' attribute's values description.
 */
typedef enum
{
  ECMA_PROPERTY_NOT_WRITABLE, /**< property's 'Writable' attribute is false */
  ECMA_PROPERTY_WRITABLE /**< property's 'Writable' attribute is true */
} ecma_property_writable_value_t;

/**
 * Property's 'Enumerable' attribute's values description.
 */
typedef enum
{
  ECMA_PROPERTY_NOT_ENUMERABLE, /**< property's 'Enumerable' attribute is false */
  ECMA_PROPERTY_ENUMERABLE /**< property's 'Enumerable' attribute is true */
} ecma_property_enumerable_value_t;

/**
 * Property's 'Configurable' attribute's values description.
 */
typedef enum
{
  ECMA_PROPERTY_NOT_CONFIGURABLE, /**< property's 'Configurable' attribute is false */
  ECMA_PROPERTY_CONFIGURABLE /**< property's 'Configurable' attribute is true */
} ecma_property_configurable_value_t;

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
  ECMA_PROPERTY_TYPE_SPECIAL, /**< internal property */
  ECMA_PROPERTY_TYPE_NAMEDDATA, /**< property is named data */
  ECMA_PROPERTY_TYPE_NAMEDACCESSOR, /**< property is named accessor */
  ECMA_PROPERTY_TYPE_VIRTUAL, /**< property is virtual */

  ECMA_PROPERTY_TYPE__MAX = ECMA_PROPERTY_TYPE_VIRTUAL, /**< highest value for property types. */
} ecma_property_types_t;

/**
 * Property type mask.
 */
#define ECMA_PROPERTY_TYPE_MASK 0x3

/**
 * Property flags base shift.
 */
#define ECMA_PROPERTY_FLAG_SHIFT 2

/**
 * Define special property type.
 */
#define ECMA_SPECIAL_PROPERTY_VALUE(type) \
  ((uint8_t) (ECMA_PROPERTY_TYPE_SPECIAL | ((type) << ECMA_PROPERTY_FLAG_SHIFT)))

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
#define ECMA_PROPERTY_TYPE_NOT_FOUND ECMA_PROPERTY_TYPE_DELETED

/**
 * Type of property not found and no more searching in the proto chain.
 */
#define ECMA_PROPERTY_TYPE_NOT_FOUND_AND_STOP ECMA_PROPERTY_TYPE_HASHMAP

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
 * Property name is a generic string.
 */
#define ECMA_PROPERTY_NAME_TYPE_STRING 3

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
#ifdef JERRY_CPOINTER_32_BIT
  jmem_cpointer_t next_property_cp; /**< next cpointer */
#endif /* JERRY_CPOINTER_32_BIT */
  ecma_property_t types[ECMA_PROPERTY_PAIR_ITEM_COUNT]; /**< two property type slot. The first represent
                                                         *   the type of this property (e.g. property pair) */
#ifdef JERRY_CPOINTER_32_BIT
  uint16_t padding; /**< an unused value */
#else /* !JERRY_CPOINTER_32_BIT */
  jmem_cpointer_t next_property_cp; /**< next cpointer */
#endif /* JERRY_CPOINTER_32_BIT */
} ecma_property_header_t;

/**
 * Pair of pointers - to property's getter and setter
 */
typedef struct
{
  jmem_cpointer_t getter_p; /**< pointer to getter object */
  jmem_cpointer_t setter_p; /**< pointer to setter object */
} ecma_getter_setter_pointers_t;

/**
 * Property data.
 */
typedef union
{
  ecma_value_t value; /**< value of a property */
#ifdef JERRY_CPOINTER_32_BIT
  jmem_cpointer_t getter_setter_pair_cp; /**< cpointer to getter setter pair */
#else /* !JERRY_CPOINTER_32_BIT */
  ecma_getter_setter_pointers_t getter_setter_pair; /**< getter setter pair */
#endif /* JERRY_CPOINTER_32_BIT */
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
  (ECMA_PROPERTY_GET_TYPE ((property_header_p)->types[0]) != ECMA_PROPERTY_TYPE_VIRTUAL \
   && (property_header_p)->types[0] != ECMA_PROPERTY_TYPE_HASHMAP)

/**
 * Returns true if the property is named property.
 */
#define ECMA_PROPERTY_IS_NAMED_PROPERTY(property) \
  (ECMA_PROPERTY_GET_TYPE (property) != ECMA_PROPERTY_TYPE_SPECIAL)

/**
 * Returns the internal property type
 */
#define ECMA_PROPERTY_GET_SPECIAL_PROPERTY_TYPE(property_p) \
  ((ecma_internal_property_id_t) (*(property_p) >> ECMA_PROPERTY_FLAG_SHIFT))

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
 * Depth limit for property search (maximum prototype chain depth).
 */
#define ECMA_PROPERTY_SEARCH_DEPTH_LIMIT 128

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
  ECMA_OBJECT_TYPE_FUNCTION = 2, /**< Function objects (15.3), created through 13.2 routine */
  ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION = 3, /**< External (host) function object */
  ECMA_OBJECT_TYPE_ARRAY = 4, /**< Array object (15.4) */
  ECMA_OBJECT_TYPE_BOUND_FUNCTION = 5, /**< Function objects (15.3), created through 15.3.4.5 routine */
  ECMA_OBJECT_TYPE_PSEUDO_ARRAY  = 6, /**< Array-like object, such as Arguments object (10.6) */
#ifndef CONFIG_DISABLE_ES2015_ARROW_FUNCTION
  ECMA_OBJECT_TYPE_ARROW_FUNCTION = 7, /**< arrow function objects */
#endif /* !CONFIG_DISABLE_ES2015_ARROW_FUNCTION */

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

  ECMA_PSEUDO_ARRAY__MAX = ECMA_PSEUDO_ARRAY_TYPEDARRAY_WITH_INFO /**< maximum value */
} ecma_pseudo_array_type_t;

/**
 * Types of lexical environments.
 */
typedef enum
{
  /* Types between 0 - 12 are ecma_object_type_t which can have a built-in flag. */

  ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE = 13, /**< declarative lexical environment */
  ECMA_LEXICAL_ENVIRONMENT_OBJECT_BOUND = 14, /**< object-bound lexical environment */
  ECMA_LEXICAL_ENVIRONMENT_THIS_OBJECT_BOUND = 15, /**< object-bound lexical environment
                                                   *   with provideThis flag */

  ECMA_LEXICAL_ENVIRONMENT_TYPE_START = ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE, /**< first lexical
                                                                               *    environment type */
  ECMA_LEXICAL_ENVIRONMENT_TYPE__MAX = ECMA_LEXICAL_ENVIRONMENT_THIS_OBJECT_BOUND /**< maximum value */
} ecma_lexical_environment_type_t;

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
 * Value for increasing or decreasing the object reference counter.
 */
#define ECMA_OBJECT_REF_ONE (1u << 6)

/**
 * Maximum value of the object reference counter (1023).
 */
#define ECMA_OBJECT_MAX_REF (0x3ffu << 6)

/**
 * Description of ECMA-object or lexical environment
 * (depending on is_lexical_environment).
 */
typedef struct
{
  /** type : 4 bit : ecma_object_type_t or ecma_lexical_environment_type_t
                     depending on ECMA_OBJECT_FLAG_BUILT_IN_OR_LEXICAL_ENV
      flags : 2 bit : ECMA_OBJECT_FLAG_BUILT_IN_OR_LEXICAL_ENV,
                      ECMA_OBJECT_FLAG_EXTENSIBLE
      refs : 10 bit (max 1023) */
  uint16_t type_flags_refs;

  /** next in the object chain maintained by the garbage collector */
  jmem_cpointer_t gc_next_cp;

  /** compressed pointer to property list or bound object */
  jmem_cpointer_t property_list_or_bound_object_cp;

  /** object prototype or outer reference */
  jmem_cpointer_t prototype_or_outer_reference_cp;
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

  /*
   * Description of extra fields. These extra fields depends on the object type.
   */
  union
  {
    ecma_built_in_props_t built_in; /**< built-in object part */

    /*
     * Description of objects with class.
     */
    struct
    {
      uint16_t class_id; /**< class id of the object */

      /*
       * Description of extra fields. These extra fields depends on the class_id.
       */
      union
      {
        ecma_value_t value; /**< value of the object (e.g. boolean, number, string, etc.) */
        uint32_t length; /**< length related property  (e.g. length of ArrayBuffer) */
      } u;
    } class_prop;

    /*
     * Description of function objects.
     */
    struct
    {
      ecma_value_t scope_cp; /**< function scope */
      ecma_value_t bytecode_cp; /**< function byte code */
    } function;

    /*
     * Description of array objects.
     */
    struct
    {
      uint32_t length; /**< length property value */
      ecma_property_t length_prop; /**< length property */
    } array;

    /*
     * Description of pseudo array objects.
     */
    struct
    {
      uint8_t type; /**< pseudo array type, e.g. Arguments, TypedArray*/
      uint8_t extra_info; /**< extra infomations about the object.
                            *  e.g. element_width_shift for typed arrays */
      union
      {
        uint16_t length; /**< for arguments: length of names */
        uint16_t class_id; /**< for typedarray: the specific class name */
      } u1;
      union
      {
        ecma_value_t lex_env_cp; /**< for arguments: lexical environment */
        ecma_value_t arraybuffer; /**< for typedarray: internal arraybuffer */
      } u2;
    } pseudo_array;

    /*
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

#ifndef CONFIG_DISABLE_ES2015_ARROW_FUNCTION

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

#endif /* !CONFIG_DISABLE_ES2015_ARROW_FUNCTION */

/**
 * Description of ECMA property descriptor
 *
 * See also: ECMA-262 v5, 8.10.
 *
 * Note:
 *      If a component of descriptor is undefined then corresponding
 *      field should contain it's default value.
 */
typedef struct
{
  /** Is [[Value]] defined? */
  unsigned int is_value_defined : 1;

  /** Is [[Get]] defined? */
  unsigned int is_get_defined : 1;

  /** Is [[Set]] defined? */
  unsigned int is_set_defined : 1;

  /** Is [[Writable]] defined? */
  unsigned int is_writable_defined : 1;

  /** [[Writable]] */
  unsigned int is_writable : 1;

  /** Is [[Enumerable]] defined? */
  unsigned int is_enumerable_defined : 1;

  /** [[Enumerable]] */
  unsigned int is_enumerable : 1;

  /** Is [[Configurable]] defined? */
  unsigned int is_configurable_defined : 1;

  /** [[Configurable]] */
  unsigned int is_configurable : 1;

  /** [[Value]] */
  ecma_value_t value;

  /** [[Get]] */
  ecma_object_t *get_p;

  /** [[Set]] */
  ecma_object_t *set_p;
} ecma_property_descriptor_t;

#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32
/**
 * Description of an ecma-number
 */
typedef float ecma_number_t;
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
#elif CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64
/**
 * Description of an ecma-number
 */
typedef double ecma_number_t;
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
#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32 */

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

/**
 * Minimum positive and maximum value of ecma-number
 */
#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32
# define ECMA_NUMBER_MIN_VALUE (FLT_MIN)
# define ECMA_NUMBER_MAX_VALUE (FLT_MAX)
#elif CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64
/**
 * Number.MAX_VALUE
 *
 * See also: ECMA_262 v5, 15.7.3.2
 */
# define ECMA_NUMBER_MAX_VALUE ((ecma_number_t) 1.7976931348623157e+308)
/**
 * Number.MIN_VALUE
 *
 * See also: ECMA_262 v5, 15.7.3.3
 */
# define ECMA_NUMBER_MIN_VALUE ((ecma_number_t) 5e-324)
#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32 */

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
 * Description of a collection's header.
 */
typedef struct
{
  /** Number of elements in the collection */
  ecma_length_t unit_number;

  /** Compressed pointer to first chunk with collection's data */
  jmem_cpointer_t first_chunk_cp;

  /** Compressed pointer to last chunk with collection's data */
  jmem_cpointer_t last_chunk_cp;
} ecma_collection_header_t;

/**
 * Description of non-first chunk in a collection's chain of chunks
 */
typedef struct
{
  /** Characters */
  lit_utf8_byte_t data[ sizeof (uint64_t) - sizeof (jmem_cpointer_t) ];

  /** Compressed pointer to next chunk */
  jmem_cpointer_t next_chunk_cp;
} ecma_collection_chunk_t;

/**
 * Identifier for ecma-string's actual data container
 */
typedef enum
{
  ECMA_STRING_CONTAINER_UINT32_IN_DESC, /**< actual data is UInt32-represeneted Number
                                             stored locally in the string's descriptor */
  ECMA_STRING_CONTAINER_MAGIC_STRING, /**< the ecma-string is equal to one of ECMA magic strings */
  ECMA_STRING_CONTAINER_MAGIC_STRING_EX, /**< the ecma-string is equal to one of external magic strings */
  ECMA_STRING_CONTAINER_HEAP_UTF8_STRING, /**< actual data is on the heap as an utf-8 (cesu8) string
                                           *   maximum size is 2^16. */
  ECMA_STRING_CONTAINER_HEAP_LONG_UTF8_STRING, /**< actual data is on the heap as an utf-8 (cesu8) string
                                                *   maximum size is 2^32. */

  ECMA_STRING_LITERAL_NUMBER, /**< a literal number which is used solely by the literal storage
                               *   so no string processing function supports this type except
                               *   the ecma_deref_ecma_string function. */

  ECMA_STRING_CONTAINER__MAX = ECMA_STRING_LITERAL_NUMBER /**< maximum value */
} ecma_string_container_t;

/**
 * Mask for getting the container of a string.
 */
#define ECMA_STRING_CONTAINER_MASK 0x7u

/**
 * Value for increasing or decreasing the reference counter.
 */
#define ECMA_STRING_REF_ONE (1u << 3)

/**
 * Maximum value of the reference counter (8191).
 */
#define ECMA_STRING_MAX_REF (0x1fffu << 3)

/**
 * Set reference counter to zero (for refs_and_container member below).
 */
#define ECMA_STRING_SET_REF_TO_ONE(refs_and_container) \
  ((uint16_t) (((refs_and_container) & ECMA_STRING_CONTAINER_MASK) | ECMA_STRING_REF_ONE))

/**
 * Returns with the container type of a string.
 */
#define ECMA_STRING_GET_CONTAINER(string_desc_p) \
  ((ecma_string_container_t) ((string_desc_p)->refs_and_container & ECMA_STRING_CONTAINER_MASK))

/**
 * Checks whether the reference counter is 1.
 */
#define ECMA_STRING_IS_REF_EQUALS_TO_ONE(string_desc_p) \
  (((string_desc_p)->refs_and_container >> 3) == 1)

/**
 * ECMA string-value descriptor
 */
typedef struct
{
  /** Reference counter for the string */
  uint16_t refs_and_container;

  /** Hash of the string (calculated from two last characters of the string) */
  lit_string_hash_t hash;

  /**
   * Actual data or identifier of it's place in container (depending on 'container' field)
   */
  union
  {
    /**
     * Actual data of an utf-8 string type
     */
    struct
    {
      uint16_t size; /**< size of this utf-8 string in bytes */
      uint16_t length; /**< length of this utf-8 string in characters */
    } utf8_string;

    lit_utf8_size_t long_utf8_string_size; /**< size of this long utf-8 string in bytes */
    uint32_t uint32_number; /**< uint32-represented number placed locally in the descriptor */
    uint32_t magic_string_id; /**< identifier of a magic string (lit_magic_string_id_t) */
    uint32_t magic_string_ex_id; /**< identifier of an external magic string (lit_magic_string_ex_id_t) */
    ecma_value_t lit_number; /**< literal number (note: not a regular string type) */
    uint32_t common_uint32_field; /**< for zeroing and comparison in some cases */
  } u;
} ecma_string_t;

/**
 * Long ECMA string-value descriptor
 */
typedef struct
{
  ecma_string_t header; /**< string header */
  lit_utf8_size_t long_utf8_string_length; /**< length of this long utf-8 string in bytes */
} ecma_long_string_t;

/**
 * Representation of a thrown value on API level.
 */
typedef struct
{
  uint32_t refs; /**< reference counter */
  ecma_value_t value; /**< referenced value */
} ecma_error_reference_t;

/**
 * Compiled byte code data.
 */
typedef struct
{
  uint16_t size;                    /**< real size >> JMEM_ALIGNMENT_LOG */
  uint16_t refs;                    /**< reference counter for the byte code */
  uint16_t status_flags;            /**< various status flags:
                                      *    CBC_CODE_FLAGS_FUNCTION flag tells whether
                                      *    the byte code is function or regular expression.
                                      *    If function, the other flags must be CBC_CODE_FLAGS...
                                      *    If regexp, the other flags must be RE_FLAG... */
} ecma_compiled_code_t;

#ifndef CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE

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

#endif /* !CONFIG_ECMA_PROPERTY_HASHMAP_DISABLE */

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

#ifndef CONFIG_ECMA_LCACHE_DISABLE

/**
 * Entry of LCache hash table
 */
typedef struct
{
  /** Pointer to a property of the object */
  ecma_property_t *prop_p;

  /** Compressed pointer to object (ECMA_NULL_POINTER marks record empty) */
  jmem_cpointer_t object_cp;

  /** Compressed pointer to property's name */
  jmem_cpointer_t prop_name_cp;
} ecma_lcache_hash_entry_t;

/**
 * Number of rows in LCache's hash table
 */
#define ECMA_LCACHE_HASH_ROWS_COUNT 128

/**
 * Number of entries in a row of LCache's hash table
 */
#define ECMA_LCACHE_HASH_ROW_LENGTH 2

#endif /* !CONFIG_ECMA_LCACHE_DISABLE */

#ifndef CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN

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

#endif /* !CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN */
/**
 * @}
 * @}
 */

#endif  /* !ECMA_GLOBALS_H */
