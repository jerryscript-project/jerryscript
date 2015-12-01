/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmatypes ECMA types
 * @{
 */

#ifndef JERRY_ECMA_GLOBALS_H
#define JERRY_ECMA_GLOBALS_H

#include "config.h"
#include "jrt.h"
#include "lit-globals.h"
#include "lit-magic-strings.h"
#include "mem-allocator.h"

/** \addtogroup compressedpointer Compressed pointer
 * @{
 */

/**
 * Ecma-pointer field is used to calculate ecma-value's address.
 *
 * Ecma-pointer contains value's shifted offset from common Ecma-pointers' base.
 * The offset is shifted right by MEM_ALIGNMENT_LOG.
 * Least significant MEM_ALIGNMENT_LOG bits of non-shifted offset are zeroes.
 */
#define ECMA_POINTER_FIELD_WIDTH MEM_CP_WIDTH

/**
 * The NULL value for compressed pointers
 */
#define ECMA_NULL_POINTER MEM_CP_NULL

/**
 * @}
 */

/**
 * Type of ecma-value
 */
typedef enum
{
  ECMA_TYPE_SIMPLE, /**< simple value */
  ECMA_TYPE_NUMBER, /**< 64-bit integer */
  ECMA_TYPE_STRING, /**< pointer to description of a string */
  ECMA_TYPE_OBJECT /**< pointer to description of an object */
} ecma_type_t;

/**
 * Simple ecma-values
 */
typedef enum
{
  /**
   * Empty value is implementation defined value, used for:
   *   - representing empty value in completion values (see also: ECMA-262 v5, 8.9 Completion specification type);
   *   - values of uninitialized immutable bindings;
   *   - values of empty register variables.
   */
  ECMA_SIMPLE_VALUE_EMPTY,
  ECMA_SIMPLE_VALUE_UNDEFINED, /**< undefined value */
  ECMA_SIMPLE_VALUE_NULL, /**< null value */
  ECMA_SIMPLE_VALUE_FALSE, /**< boolean false */
  ECMA_SIMPLE_VALUE_TRUE, /**< boolean true */
  ECMA_SIMPLE_VALUE_ARRAY_REDIRECT, /**< implementation defined value for an array's elements that exist,
                                         but are stored directly in the array's property list
                                         (used for array elements with non-default attribute values) */
  ECMA_SIMPLE_VALUE_ARRAY_HOLE, /**< array hole, used for initialization of an array literal */
  ECMA_SIMPLE_VALUE__COUNT /** count of simple ecma-values */
} ecma_simple_value_t;

/**
 * Type of ecma-property
 */
typedef enum
{
  ECMA_PROPERTY_NAMEDDATA, /**< named data property */
  ECMA_PROPERTY_NAMEDACCESSOR, /**< named accessor property */
  ECMA_PROPERTY_INTERNAL /**< internal property */
} ecma_property_type_t;

/**
 * Type of block evaluation (completion) result.
 *
 * See also: ECMA-262 v5, 8.9.
 */
typedef enum
{
  ECMA_COMPLETION_TYPE_NORMAL, /**< default completion */
  ECMA_COMPLETION_TYPE_RETURN, /**< completion with return */
  ECMA_COMPLETION_TYPE_JUMP, /**< implementation-defined completion type
                              *   for jump statements (break, continue)
                              *   that require completion of one or several
                              *   statements, before performing related jump.
                              *
                              *   For example, 'break' in the following code
                              *   requires to return from opfunc_with handler
                              *   before performing jump to the loop end:
                              *
                              *     for (var i = 0; i < 10; i++)
                              *     {
                              *        with (obj)
                              *        {
                              *          break;
                              *        }
                              *     }
                              */
  ECMA_COMPLETION_TYPE_THROW, /**< completion with throw */
  ECMA_COMPLETION_TYPE_META /**< implementation-defined completion type
                                 for meta opcode */
} ecma_completion_type_t;

/**
 * Description of an ecma-value
 *
 * Bit-field structure: type (2) | value (ECMA_POINTER_FIELD_WIDTH)
 */
typedef uint32_t ecma_value_t;

/**
 * Value type (ecma_type_t)
 */
#define ECMA_VALUE_TYPE_POS (0)
#define ECMA_VALUE_TYPE_WIDTH (2)

/**
 * Simple value (ecma_simple_value_t) or compressed pointer to value (depending on value_type)
 */
#define ECMA_VALUE_VALUE_POS (ECMA_VALUE_TYPE_POS + \
                              ECMA_VALUE_TYPE_WIDTH)
#define ECMA_VALUE_VALUE_WIDTH (ECMA_POINTER_FIELD_WIDTH)

/**
 * Size of ecma value description, in bits
 */
#define ECMA_VALUE_SIZE (ECMA_VALUE_VALUE_POS + ECMA_VALUE_VALUE_WIDTH)

/**
 * Description of a block completion value
 *
 * See also: ECMA-262 v5, 8.9.
 *
 *                                               value (16)
 * Bit-field structure: type (8) | padding (8) <
 *                                               break / continue target
 */
typedef uint32_t ecma_completion_value_t;

/**
 * Value
 *
 * Used for normal, return, throw and exit completion types.
 */
#define ECMA_COMPLETION_VALUE_VALUE_POS (0)
#define ECMA_COMPLETION_VALUE_VALUE_WIDTH (ECMA_VALUE_SIZE)

/**
 * Break / continue jump target
 */
#define ECMA_COMPLETION_VALUE_TARGET_POS (0)
#define ECMA_COMPLETION_VALUE_TARGET_WIDTH ((uint32_t) sizeof (vm_instr_counter_t) * JERRY_BITSINBYTE)

/**
 * Type (ecma_completion_type_t)
 */
#define ECMA_COMPLETION_VALUE_TYPE_POS (JERRY_MAX (JERRY_ALIGNUP (ECMA_COMPLETION_VALUE_VALUE_POS + \
                                                                  ECMA_COMPLETION_VALUE_VALUE_WIDTH, \
                                                                  JERRY_BITSINBYTE), \
                                                   JERRY_ALIGNUP (ECMA_COMPLETION_VALUE_TARGET_POS + \
                                                                  ECMA_COMPLETION_VALUE_TARGET_WIDTH, \
                                                                  JERRY_BITSINBYTE)))
#define ECMA_COMPLETION_VALUE_TYPE_WIDTH (8)

/**
 * Size of ecma completion value description, in bits
 */
#define ECMA_COMPLETION_VALUE_SIZE (ECMA_COMPLETION_VALUE_TYPE_POS + \
                                    ECMA_COMPLETION_VALUE_TYPE_WIDTH)

/**
 * Label
 *
 * Used for break and continue completion types.
 */
typedef struct
{
  /** Target's offset */
  uint32_t offset;

  /** Levels to label left */
  uint32_t depth;
} ecma_label_descriptor_t;

/**
 * Internal properties' identifiers.
 */
typedef enum
{
  ECMA_INTERNAL_PROPERTY_CLASS, /**< [[Class]] */
  ECMA_INTERNAL_PROPERTY_PROTOTYPE, /**< [[Prototype]] */
  ECMA_INTERNAL_PROPERTY_EXTENSIBLE, /**< [[Extensible]] */
  ECMA_INTERNAL_PROPERTY_SCOPE, /**< [[Scope]] */
  ECMA_INTERNAL_PROPERTY_PARAMETERS_MAP, /**< [[ParametersMap]] */
  ECMA_INTERNAL_PROPERTY_CODE_BYTECODE, /**< first part of [[Code]] - compressed pointer to bytecode array */
  ECMA_INTERNAL_PROPERTY_CODE_FLAGS_AND_OFFSET, /**< second part of [[Code]] - offset in bytecode array and code flags
                                                 *   (see also: ecma_pack_code_internal_property_value) */
  ECMA_INTERNAL_PROPERTY_NATIVE_CODE, /**< native handler location descriptor */
  ECMA_INTERNAL_PROPERTY_NATIVE_HANDLE, /**< native handle associated with an object */
  ECMA_INTERNAL_PROPERTY_FREE_CALLBACK, /**< object's native free callback */
  ECMA_INTERNAL_PROPERTY_FORMAL_PARAMETERS, /**< [[FormalParameters]] */
  ECMA_INTERNAL_PROPERTY_PRIMITIVE_STRING_VALUE, /**< [[Primitive value]] for String objects */
  ECMA_INTERNAL_PROPERTY_PRIMITIVE_NUMBER_VALUE, /**< [[Primitive value]] for Number objects */
  ECMA_INTERNAL_PROPERTY_PRIMITIVE_BOOLEAN_VALUE, /**< [[Primitive value]] for Boolean objects */

  /** Part of an array, that is indexed by numbers */
  ECMA_INTERNAL_PROPERTY_NUMBER_INDEXED_ARRAY_VALUES,

  /** Part of an array, that is indexed by strings */
  ECMA_INTERNAL_PROPERTY_STRING_INDEXED_ARRAY_VALUES,

  /** Implementation-defined identifier of built-in object */
  ECMA_INTERNAL_PROPERTY_BUILT_IN_ID,

  /** Implementation-defined identifier of built-in routine
      that corresponds to a built-in function object
      ([[Built-in routine's description]]) */
  ECMA_INTERNAL_PROPERTY_BUILT_IN_ROUTINE_DESC,

  /** Identifier of implementation-defined extension object */
  ECMA_INTERNAL_PROPERTY_EXTENSION_ID,

  /** Bound function internal properties **/
  ECMA_INTERNAL_PROPERTY_BOUND_FUNCTION_TARGET_FUNCTION,
  ECMA_INTERNAL_PROPERTY_BOUND_FUNCTION_BOUND_THIS,
  ECMA_INTERNAL_PROPERTY_BOUND_FUNCTION_BOUND_ARGS,

  /**
   * Bit-mask of non-instantiated built-in's properties (bits 0-31)
   */
  ECMA_INTERNAL_PROPERTY_NON_INSTANTIATED_BUILT_IN_MASK_0_31,

  /**
   * Bit-mask of non-instantiated built-in's properties (bits 32-63)
   */
  ECMA_INTERNAL_PROPERTY_NON_INSTANTIATED_BUILT_IN_MASK_32_63,

  /**
   * RegExp bytecode array
   */
  ECMA_INTERNAL_PROPERTY_REGEXP_BYTECODE,

  /**
   * Number of internal properties' types
   */
  ECMA_INTERNAL_PROPERTY__COUNT
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
 * Width of internal property type field's width
 */
#define ECMA_PROPERTY_INTERNAL_PROPERTY_TYPE_WIDTH   (5)

/**
 * Pair of pointers - to property's getter and setter
 */
typedef struct
{
  mem_cpointer_t getter_p : ECMA_POINTER_FIELD_WIDTH; /**< pointer to getter object */
  mem_cpointer_t setter_p : ECMA_POINTER_FIELD_WIDTH; /**< pointer to setter object */
} ecma_getter_setter_pointers_t;

/**
 * Description of ecma-property
 */
typedef struct __attr_packed___ ecma_property_t
{
  /** Property's type (ecma_property_type_t) */
  unsigned int type : 2;

  /** Compressed pointer to next property */
  mem_cpointer_t next_property_p : ECMA_POINTER_FIELD_WIDTH;

  /** Property's details (depending on Type) */
  union
  {
    /** Description of named data property */
    struct __attr_packed___ ecma_named_data_property_t
    {
      /** Value */
      ecma_value_t value : ECMA_VALUE_SIZE;

      /** Compressed pointer to property's name (pointer to String) */
      mem_cpointer_t name_p : ECMA_POINTER_FIELD_WIDTH;

      /** Flag indicating whether the property is registered in LCache */
      unsigned int is_lcached : 1;

      /** Attribute 'Writable' (ecma_property_writable_value_t) */
      unsigned int writable : 1;

      /** Attribute 'Enumerable' (ecma_property_enumerable_value_t) */
      unsigned int enumerable : 1;

      /** Attribute 'Configurable' (ecma_property_configurable_value_t) */
      unsigned int configurable : 1;
    } named_data_property;

    /** Description of named accessor property */
    struct __attr_packed___ ecma_named_accessor_property_t
    {
      /** Compressed pointer to property's name (pointer to String) */
      mem_cpointer_t name_p : ECMA_POINTER_FIELD_WIDTH;

      /** Attribute 'Enumerable' (ecma_property_enumerable_value_t) */
      unsigned int enumerable : 1;

      /** Attribute 'Configurable' (ecma_property_configurable_value_t) */
      unsigned int configurable : 1;

      /** Flag indicating whether the property is registered in LCache */
      unsigned int is_lcached : 1;

      /** Compressed pointer to pair of pointers - to property's getter and setter */
      mem_cpointer_t getter_setter_pair_cp : ECMA_POINTER_FIELD_WIDTH;
    } named_accessor_property;

    /** Description of internal property */
    struct __attr_packed___ ecma_internal_property_t
    {
      /** Internal property's type */
      unsigned int type : ECMA_PROPERTY_INTERNAL_PROPERTY_TYPE_WIDTH;

      /** Value (may be a compressed pointer) */
      uint32_t value;
    } internal_property;
  } u;
} ecma_property_t;

/**
 * Types of lexical environments
 */
typedef enum
{
  ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE, /**< declarative lexical environment */
  ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND /**< object-bound lexical environment */
} ecma_lexical_environment_type_t;

/**
 * Internal object types
 *
 * Warning:
 *         definition order is significant (see also dispatch tables in libecmaobjects/ecma-objects.c)
 */
typedef enum
{
  ECMA_OBJECT_TYPE_GENERAL, /**< all objects that are not String (15.5), Function (15.3),
                                 Arguments (10.6), Array (15.4) specification-defined objects
                                 and not host objects */
  ECMA_OBJECT_TYPE_STRING, /**< String objects (15.5) */
  ECMA_OBJECT_TYPE_FUNCTION, /**< Function objects (15.3), created through 13.2 routine */
  ECMA_OBJECT_TYPE_EXTERNAL_FUNCTION, /**< External (host) function object */
  ECMA_OBJECT_TYPE_BOUND_FUNCTION, /**< Function objects (15.3), created through 15.3.4.5 routine */
  ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION, /** One of built-in functions described in section 15
                                          of ECMA-262 v5 specification */
  ECMA_OBJECT_TYPE_ARGUMENTS, /**< Arguments object (10.6) */
  ECMA_OBJECT_TYPE_ARRAY /**< Array object (15.4) */
  // ECMA_OBJECT_TYPE_HOST /**< Host object */
} ecma_object_type_t;

/**
 * Description of ECMA-object or lexical environment
 * (depending on is_lexical_environment).
 */
typedef struct ecma_object_t
{
/* Common part for objects and lexical environments */

/**
 * Compressed pointer to property list
 */
#define ECMA_OBJECT_PROPERTIES_OR_BOUND_OBJECT_CP_POS   (0)
#define ECMA_OBJECT_PROPERTIES_OR_BOUND_OBJECT_CP_WIDTH (ECMA_POINTER_FIELD_WIDTH)

/**
 * Flag indicating whether it is a general object (false)
 * or a lexical environment (true)
 */
#define ECMA_OBJECT_IS_LEXICAL_ENVIRONMENT_POS (ECMA_OBJECT_PROPERTIES_OR_BOUND_OBJECT_CP_POS + \
                                                ECMA_OBJECT_PROPERTIES_OR_BOUND_OBJECT_CP_WIDTH)
#define ECMA_OBJECT_IS_LEXICAL_ENVIRONMENT_WIDTH (1)

/**
 * Reference counter of the object, i.e. number of references
 * to the object from stack variables.
 */
#define ECMA_OBJECT_GC_REFS_POS (ECMA_OBJECT_IS_LEXICAL_ENVIRONMENT_POS + \
                                 ECMA_OBJECT_IS_LEXICAL_ENVIRONMENT_WIDTH)
#define ECMA_OBJECT_GC_REFS_WIDTH (CONFIG_ECMA_REFERENCE_COUNTER_WIDTH)

/**
 * Compressed pointer to next object in the global list of objects with same generation.
 */
#define ECMA_OBJECT_GC_NEXT_CP_POS (ECMA_OBJECT_GC_REFS_POS + \
                                    ECMA_OBJECT_GC_REFS_WIDTH)
#define ECMA_OBJECT_GC_NEXT_CP_WIDTH (ECMA_POINTER_FIELD_WIDTH)

/**
 * Marker that is set if the object was visited during graph traverse.
 */
#define ECMA_OBJECT_GC_VISITED_POS (ECMA_OBJECT_GC_NEXT_CP_POS + \
                                    ECMA_OBJECT_GC_NEXT_CP_WIDTH)
#define ECMA_OBJECT_GC_VISITED_WIDTH (1)


/* Objects' only part */

/**
 * Attribute 'Extensible'
 */
#define ECMA_OBJECT_OBJ_EXTENSIBLE_POS (ECMA_OBJECT_GC_VISITED_POS + \
                                        ECMA_OBJECT_GC_VISITED_WIDTH)
#define ECMA_OBJECT_OBJ_EXTENSIBLE_WIDTH (1)

/**
 * Implementation internal object type (ecma_object_type_t)
 */
#define ECMA_OBJECT_OBJ_TYPE_POS (ECMA_OBJECT_OBJ_EXTENSIBLE_POS + \
                                  ECMA_OBJECT_OBJ_EXTENSIBLE_WIDTH)
#define ECMA_OBJECT_OBJ_TYPE_WIDTH (4)

/**
 * Compressed pointer to prototype object (ecma_object_t)
 */
#define ECMA_OBJECT_OBJ_PROTOTYPE_OBJECT_CP_POS (ECMA_OBJECT_OBJ_TYPE_POS + \
                                                 ECMA_OBJECT_OBJ_TYPE_WIDTH)
#define ECMA_OBJECT_OBJ_PROTOTYPE_OBJECT_CP_WIDTH (ECMA_POINTER_FIELD_WIDTH)

/**
 * Flag indicating whether the object is a built-in object
 */
#define ECMA_OBJECT_OBJ_IS_BUILTIN_POS (ECMA_OBJECT_OBJ_PROTOTYPE_OBJECT_CP_POS + \
                                        ECMA_OBJECT_OBJ_PROTOTYPE_OBJECT_CP_WIDTH)
#define ECMA_OBJECT_OBJ_IS_BUILTIN_WIDTH (1)

/**
 * Size of structure for objects
 */
#define ECMA_OBJECT_OBJ_TYPE_SIZE (ECMA_OBJECT_OBJ_IS_BUILTIN_POS + \
                                   ECMA_OBJECT_OBJ_IS_BUILTIN_WIDTH)


/* Lexical environments' only part */

/**
 * Type of lexical environment (ecma_lexical_environment_type_t).
 */
#define ECMA_OBJECT_LEX_ENV_TYPE_POS (ECMA_OBJECT_GC_VISITED_POS + \
                                        ECMA_OBJECT_GC_VISITED_WIDTH)
#define ECMA_OBJECT_LEX_ENV_TYPE_WIDTH (1)

/**
 * Compressed pointer to outer lexical environment
 */
#define ECMA_OBJECT_LEX_ENV_OUTER_REFERENCE_CP_POS (ECMA_OBJECT_LEX_ENV_TYPE_POS + \
                                                    ECMA_OBJECT_LEX_ENV_TYPE_WIDTH)
#define ECMA_OBJECT_LEX_ENV_OUTER_REFERENCE_CP_WIDTH (ECMA_POINTER_FIELD_WIDTH)

/**
 * 'provideThis' property of object-bound lexical environments
 */
#define ECMA_OBJECT_LEX_ENV_PROVIDE_THIS_POS (ECMA_OBJECT_LEX_ENV_OUTER_REFERENCE_CP_POS + \
                                              ECMA_OBJECT_LEX_ENV_OUTER_REFERENCE_CP_WIDTH)
#define ECMA_OBJECT_LEX_ENV_PROVIDE_THIS_WIDTH (1)

/**
 * Size of structure for lexical environments
 */
#define ECMA_OBJECT_LEX_ENV_TYPE_SIZE (ECMA_OBJECT_LEX_ENV_PROVIDE_THIS_POS + \
                                       ECMA_OBJECT_LEX_ENV_PROVIDE_THIS_WIDTH)

  uint64_t container; /**< container for fields described above */
} ecma_object_t;


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

  /** Is [[Enumerable]] defined? */
  unsigned int is_enumerable_defined : 1;

  /** Is [[Configurable]] defined? */
  unsigned int is_configurable_defined : 1;

  /** [[Value]] */
  ecma_value_t value;

  /** [[Get]] */
  ecma_object_t *get_p;

  /** [[Set]] */
  ecma_object_t *set_p;

  /** [[Writable]] */
  bool is_writable;

  /** [[Enumerable]] */
  bool is_enumerable;

  /** [[Configurable]] */
  bool is_configurable;
} ecma_property_descriptor_t;

#if CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT32
/**
 * Description of an ecma-number
 */
typedef float ecma_number_t;
#define DOUBLE_TO_ECMA_NUMBER_T(value) static_cast<ecma_number_t> (value)

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
#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64 */

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
# define ECMA_NUMBER_MAX_VALUE ((ecma_number_t)1.7976931348623157e+308)
/**
 * Number.MIN_VALUE
 *
 * See also: ECMA_262 v5, 15.7.3.3
 */
# define ECMA_NUMBER_MIN_VALUE ((ecma_number_t)5e-324)
#endif /* CONFIG_ECMA_NUMBER_TYPE == CONFIG_ECMA_NUMBER_FLOAT64 */

/**
 * Euler number
 */
#define ECMA_NUMBER_E  ((ecma_number_t)2.7182818284590452354)

/**
 * Natural logarithm of 10
 */
#define ECMA_NUMBER_LN10 ((ecma_number_t)2.302585092994046)

/**
 * Natural logarithm of 2
 */
#define ECMA_NUMBER_LN2 ((ecma_number_t)0.6931471805599453)

/**
 * Logarithm base 2 of the Euler number
 */
#define ECMA_NUMBER_LOG2E ((ecma_number_t)1.4426950408889634)

/**
 * Logarithm base 10 of the Euler number
 */
#define ECMA_NUMBER_LOG10E ((ecma_number_t)0.4342944819032518)

/**
 * Pi number
 */
#define ECMA_NUMBER_PI  ((ecma_number_t)3.1415926535897932)

/**
 * Square root of 0.5
 */
#define ECMA_NUMBER_SQRT_1_2  ((ecma_number_t)0.7071067811865476)

/**
 * Square root of 2
 */
#define ECMA_NUMBER_SQRT2  ((ecma_number_t)1.4142135623730951)

/**
 * Maximum number of characters in string representation of ecma-number
 */
#define ECMA_MAX_CHARS_IN_STRINGIFIED_NUMBER 64

/**
 * Maximum number of characters in string representation of ecma-uint32
 */
#define ECMA_MAX_CHARS_IN_STRINGIFIED_UINT32 32

/**
 * Maximum value of valid array index
 *
 * See also:
 *          ECMA-262 v5, 15.4
 */
#define ECMA_MAX_VALUE_OF_VALID_ARRAY_INDEX ((uint32_t) (-1))

/**
 * Description of a collection's header.
 */
typedef struct
{
  /** Number of elements in the collection */
  ecma_length_t unit_number;

  /** Compressed pointer to first chunk with collection's data */
  mem_cpointer_t first_chunk_cp;

  /** Compressed pointer to last chunk with collection's data */
  mem_cpointer_t last_chunk_cp;
} ecma_collection_header_t;

/**
 * Description of non-first chunk in a collection's chain of chunks
 */
typedef struct
{
  /** Characters */
  lit_utf8_byte_t data[ sizeof (uint64_t) - sizeof (mem_cpointer_t) ];

  /** Compressed pointer to next chunk */
  mem_cpointer_t next_chunk_cp;
} ecma_collection_chunk_t;

/**
 * Identifier for ecma-string's actual data container
 */
typedef enum
{
  ECMA_STRING_CONTAINER_LIT_TABLE, /**< actual data is in literal table */
  ECMA_STRING_CONTAINER_HEAP_CHUNKS, /**< actual data is on the heap
                                          in a ecma_collection_chunk_t chain */
  ECMA_STRING_CONTAINER_HEAP_NUMBER, /**< actual data is on the heap as a ecma_number_t */
  ECMA_STRING_CONTAINER_UINT32_IN_DESC, /**< actual data is UInt32-represeneted Number
                                             stored locally in the string's descriptor */
  ECMA_STRING_CONTAINER_MAGIC_STRING, /**< the ecma-string is equal to one of ECMA magic strings */
  ECMA_STRING_CONTAINER_MAGIC_STRING_EX /**< the ecma-string is equal to one of external magic strings */
} ecma_string_container_t;

/**
 * ECMA string-value descriptor
 */
typedef struct ecma_string_t
{
  /** Reference counter for the string */
  unsigned int refs : CONFIG_ECMA_REFERENCE_COUNTER_WIDTH;

  /** Flag indicating whether the string descriptor is placed
    * in a stack variable (not in the heap) */
  unsigned int is_stack_var : 1;

  /** Where the string's data is placed (ecma_string_container_t) */
  uint8_t container;

  /** Hash of the string (calculated from two last characters of the string) */
  lit_string_hash_t hash;

  /**
   * Actual data or identifier of it's place in container (depending on 'container' field)
   */
  union
  {
    /** Index of string in literal table */
    lit_cpointer_t lit_cp;

    /** Compressed pointer to an ecma_collection_header_t */
    mem_cpointer_t collection_cp : ECMA_POINTER_FIELD_WIDTH;

    /** Compressed pointer to an ecma_number_t */
    mem_cpointer_t number_cp : ECMA_POINTER_FIELD_WIDTH;

    /** UInt32-represented number placed locally in the descriptor */
    uint32_t uint32_number;

    /** Identifier of magic string */
    lit_magic_string_id_t magic_string_id;

    /** Identifier of external magic string */
    lit_magic_string_ex_id_t magic_string_ex_id;

    /** For zeroing and comparison in some cases */
    uint32_t common_field;
  } u;
} ecma_string_t;

/**
 * Representation for native external pointer
 */
typedef uintptr_t ecma_external_pointer_t;

/**
 * @}
 */

#endif  /* JERRY_ECMA_GLOBALS_H */

/**
 * @}
 * @}
 */
