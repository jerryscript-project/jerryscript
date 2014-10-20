/* Copyright 2014 Samsung Electronics Co., Ltd.
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
#include "globals.h"
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
#define ECMA_POINTER_FIELD_WIDTH MEM_COMPRESSED_POINTER_WIDTH

/**
 * The NULL value for compressed pointers
 */
#define ECMA_NULL_POINTER MEM_COMPRESSED_POINTER_NULL

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
  ECMA_COMPLETION_TYPE_NORMAL, /**< default block completion */
  ECMA_COMPLETION_TYPE_RETURN, /**< block completed with return */
  ECMA_COMPLETION_TYPE_BREAK, /**< block completed with break */
  ECMA_COMPLETION_TYPE_CONTINUE, /**< block completed with continue */
#ifdef CONFIG_ECMA_EXCEPTION_SUPPORT
  ECMA_COMPLETION_TYPE_THROW, /**< block completed with throw */
#endif /* CONFIG_ECMA_EXCEPTION_SUPPORT */
  ECMA_COMPLETION_TYPE_EXIT, /**< implementation-defined completion type
                                  for finishing script execution */
  ECMA_COMPLETION_TYPE_META /**< implementation-defined completion type
                                 for meta opcode */
} ecma_completion_type_t;

/**
 * Description of an ecma-value
 */
typedef struct
{
  /** Value type (ecma_type_t) */
  unsigned int value_type : 2;

  /**
   * Simple value (ecma_simple_value_t) or compressed pointer to value (depending on value_type)
   */
  unsigned int value : ECMA_POINTER_FIELD_WIDTH;
} __packed ecma_value_t;

/**
 * Description of a block completion value
 *
 * See also: ECMA-262 v5, 8.9.
 */
typedef struct
{
  /** Type (ecma_completion_type_t) */
  uint8_t type;

  /** Just padding for the structure */
  uint8_t padding;

  union
  {
    /**
      * Value
      *
      * Used for normal, return, throw and exit completion types.
      */
    ecma_value_t value;

    /**
      * Label
      *
      * Used for break and continue completion types.
      */
    uint16_t label_desc_cp;
  } u;
} ecma_completion_value_t;

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
  ECMA_INTERNAL_PROPERTY_CODE, /**< [[Code]] */
  ECMA_INTERNAL_PROPERTY_FORMAL_PARAMETERS, /**< [[FormalParameters]] */
  ECMA_INTERNAL_PROPERTY_PRIMITIVE_STRING_VALUE, /**< [[Primitive value]] for String objects */
  ECMA_INTERNAL_PROPERTY_PRIMITIVE_NUMBER_VALUE, /**< [[Primitive value]] for Number objects */
  ECMA_INTERNAL_PROPERTY_PRIMITIVE_BOOLEAN_VALUE, /**< [[Primitive value]] for Boolean objects */

  /** provideThis property of lexical environment */
  ECMA_INTERNAL_PROPERTY_PROVIDE_THIS,

  /** binding object of lexical environment */
  ECMA_INTERNAL_PROPERTY_BINDING_OBJECT,

  /** Part of an array, that is indexed by numbers */
  ECMA_INTERNAL_PROPERTY_NUMBER_INDEXED_ARRAY_VALUES,

  /** Part of an array, that is indexed by strings */
  ECMA_INTERNAL_PROPERTY_STRING_INDEXED_ARRAY_VALUES,

  /** Implementation-defined identifier of built-in object */
  ECMA_INTERNAL_PROPERTY_BUILT_IN_ID,

  /** Implementation-defined identifier of built-in routine
      that corresponds to a built-in function object
      ([[Built-in routine ID]]) */
  ECMA_INTERNAL_PROPERTY_BUILT_IN_ROUTINE_ID,

  /**
   * Bit-mask of non-instantiated built-in's properties (bits 0-31)
   */
  ECMA_INTERNAL_PROPERTY_NON_INSTANTIATED_BUILT_IN_MASK_0_31,

  /**
   * Bit-mask of non-instantiated built-in's properties (bits 32-63)
   */
  ECMA_INTERNAL_PROPERTY_NON_INSTANTIATED_BUILT_IN_MASK_32_63
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
 * Description of ecma-property
 */
typedef struct ecma_property_t
{
  /** Property's type (ecma_property_type_t) */
  unsigned int type : 2;

  /** Compressed pointer to next property */
  unsigned int next_property_p : ECMA_POINTER_FIELD_WIDTH;

  /** Property's details (depending on Type) */
  union
  {
    /** Description of named data property */
    struct __packed ecma_named_data_property_t
    {
      /** Compressed pointer to property's name (pointer to String) */
      unsigned int name_p : ECMA_POINTER_FIELD_WIDTH;

      /** Attribute 'Writable' (ecma_property_writable_value_t) */
      unsigned int writable : 1;

      /** Attribute 'Enumerable' (ecma_property_enumerable_value_t) */
      unsigned int enumerable : 1;

      /** Attribute 'Configurable' (ecma_property_configurable_value_t) */
      unsigned int configurable : 1;

      /** Value */
      ecma_value_t value;
    } named_data_property;

    /** Description of named accessor property */
    struct __packed ecma_named_accessor_property_t
    {
      /** Compressed pointer to property's name (pointer to String) */
      unsigned int name_p : ECMA_POINTER_FIELD_WIDTH;

      /** Attribute 'Enumerable' (ecma_property_enumerable_value_t) */
      unsigned int enumerable : 1;

      /** Attribute 'Configurable' (ecma_property_configurable_value_t) */
      unsigned int configurable : 1;

      /** Compressed pointer to property's getter */
      unsigned int get_p : ECMA_POINTER_FIELD_WIDTH;

      /** Compressed pointer to property's setter */
      unsigned int set_p : ECMA_POINTER_FIELD_WIDTH;
    } named_accessor_property;

    /** Description of internal property */
    struct __packed ecma_internal_property_t
    {
      /** Internal property's type */
      unsigned int type : 5;

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
 */
typedef enum
{
  ECMA_OBJECT_TYPE_GENERAL, /**< all objects that are not String (15.5), Function (15.3),
                                 Arguments (10.6), Array (15.4) specification-defined objects
                                 and not host objects */
  ECMA_OBJECT_TYPE_STRING, /**< String objects (15.5) */
  ECMA_OBJECT_TYPE_FUNCTION, /**< Function objects (15.3), created through 13.2 routine */
  ECMA_OBJECT_TYPE_BOUND_FUNCTION, /**< Function objects (15.3), created through 15.3.4.5 routine */
  ECMA_OBJECT_TYPE_BUILT_IN_FUNCTION, /** One of built-in functions described in section 15
                                          of ECMA-262 v5 specification */
  ECMA_OBJECT_TYPE_ARGUMENTS, /**< Arguments object (10.6) */
  ECMA_OBJECT_TYPE_ARRAY, /**< Array object (15.4) */
  ECMA_OBJECT_TYPE_HOST /**< Host object */
} ecma_object_type_t;

/**
 * Description of ECMA-object or lexical environment
 * (depending on is_lexical_environment).
 */
typedef struct
{
/* Common part for objects and lexical environments */

/**
 * Compressed pointer to property list
 */
#define ECMA_OBJECT_PROPERTIES_CP_POS   (0)
#define ECMA_OBJECT_PROPERTIES_CP_WIDTH (ECMA_POINTER_FIELD_WIDTH)

/**
 * Flag indicating whether it is a general object (false)
 * or a lexical environment (true)
 */
#define ECMA_OBJECT_IS_LEXICAL_ENVIRONMENT_POS (ECMA_OBJECT_PROPERTIES_CP_POS + \
                                                ECMA_OBJECT_PROPERTIES_CP_WIDTH)
#define ECMA_OBJECT_IS_LEXICAL_ENVIRONMENT_WIDTH (1)

/**
 * Reference counter of the object, i.e. number of references
 * to the object from stack variables.
 */
#define ECMA_OBJECT_GC_REFS_POS (ECMA_OBJECT_IS_LEXICAL_ENVIRONMENT_POS + \
                                 ECMA_OBJECT_IS_LEXICAL_ENVIRONMENT_WIDTH)
#define ECMA_OBJECT_GC_REFS_WIDTH (CONFIG_ECMA_REFERENCE_COUNTER_WIDTH)

/**
 * Identifier of GC generation.
 */
#define ECMA_OBJECT_GC_GENERATION_POS (ECMA_OBJECT_GC_REFS_POS + \
                                       ECMA_OBJECT_GC_REFS_WIDTH)
#define ECMA_OBJECT_GC_GENERATION_WIDTH (2)

/**
 * Compressed pointer to next object in the global list of objects with same generation.
 */
#define ECMA_OBJECT_GC_NEXT_CP_POS (ECMA_OBJECT_GC_GENERATION_POS + \
                                    ECMA_OBJECT_GC_GENERATION_WIDTH)
#define ECMA_OBJECT_GC_NEXT_CP_WIDTH (ECMA_POINTER_FIELD_WIDTH)

/**
 * Marker that is set if the object was visited during graph traverse.
 */
#define ECMA_OBJECT_GC_VISITED_POS (ECMA_OBJECT_GC_NEXT_CP_POS + \
                                    ECMA_OBJECT_GC_NEXT_CP_WIDTH)
#define ECMA_OBJECT_GC_VISITED_WIDTH (1)

/**
 * Flag indicating that the object may reference objects of younger generations in its properties.
 */
#define ECMA_OBJECT_GC_MAY_REF_YOUNGER_OBJECTS_POS (ECMA_OBJECT_GC_VISITED_POS + \
                                                    ECMA_OBJECT_GC_VISITED_WIDTH)
#define ECMA_OBJECT_GC_MAY_REF_YOUNGER_OBJECTS_WIDTH (1)


/* Objects' only part */

/**
 * Attribute 'Extensible'
 */
#define ECMA_OBJECT_OBJ_EXTENSIBLE_POS (ECMA_OBJECT_GC_MAY_REF_YOUNGER_OBJECTS_POS + \
                                        ECMA_OBJECT_GC_MAY_REF_YOUNGER_OBJECTS_WIDTH)
#define ECMA_OBJECT_OBJ_EXTENSIBLE_WIDTH (1)

/**
 * Implementation internal object type (ecma_object_type_t)
 */
#define ECMA_OBJECT_OBJ_TYPE_POS (ECMA_OBJECT_OBJ_EXTENSIBLE_POS + \
                                  ECMA_OBJECT_OBJ_EXTENSIBLE_WIDTH)
#define ECMA_OBJECT_OBJ_TYPE_WIDTH (3)

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
#define ECMA_OBJECT_LEX_ENV_TYPE_POS (ECMA_OBJECT_GC_MAY_REF_YOUNGER_OBJECTS_POS + \
                                      ECMA_OBJECT_GC_MAY_REF_YOUNGER_OBJECTS_WIDTH)
#define ECMA_OBJECT_LEX_ENV_TYPE_WIDTH (1)

/**
 * Compressed pointer to outer lexical environment
 */
#define ECMA_OBJECT_LEX_ENV_OUTER_REFERENCE_CP_POS (ECMA_OBJECT_LEX_ENV_TYPE_POS + \
                                                    ECMA_OBJECT_LEX_ENV_TYPE_WIDTH)
#define ECMA_OBJECT_LEX_ENV_OUTER_REFERENCE_CP_WIDTH (ECMA_POINTER_FIELD_WIDTH)

/**
 * Size of structure for lexical environments
 */
#define ECMA_OBJECT_LEX_ENV_TYPE_SIZE (ECMA_OBJECT_LEX_ENV_OUTER_REFERENCE_CP_POS + \
                                       ECMA_OBJECT_LEX_ENV_OUTER_REFERENCE_CP_WIDTH)

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
  ecma_object_t* get_p;

  /** [[Set]] */
  ecma_object_t* set_p;

  /** [[Writable]] */
  ecma_property_writable_value_t writable;

  /** [[Enumerable]] */
  ecma_property_enumerable_value_t enumerable;

  /** [[Configurable]] */
  ecma_property_configurable_value_t configurable;
} ecma_property_descriptor_t;

#ifdef CONFIG_ECMA_CHAR_ASCII
/**
 * Description of an ecma-character
 */
typedef uint8_t ecma_char_t;
#elif defined (CONFIG_ECMA_CHAR_UTF16)
/**
 * Description of an ecma-character
 */
typedef uint16_t ecma_char_t;
#else /* !CONFIG_ECMA_CHAR_ASCII && !CONFIG_ECMA_CHAR_UTF16 */
# error "!CONFIG_ECMA_CHAR_ASCII && !CONFIG_ECMA_CHAR_UTF16"
#endif /* !CONFIG_ECMA_CHAR_ASCII && !CONFIG_ECMA_CHAR_UTF16 */

#ifdef CONFIG_ECMA_NUMBER_FLOAT32
/**
 * Description of an ecma-number
 */
typedef float ecma_number_t;
#elif defined (CONFIG_ECMA_NUMBER_FLOAT64)
/**
 * Description of an ecma-number
 */
typedef double ecma_number_t;
#else /* !CONFIG_ECMA_NUMBER_FLOAT32 && !CONFIG_ECMA_NUMBER_FLOAT64 */
#error "!CONFIG_ECMA_NUMBER_FLOAT32 && !CONFIG_ECMA_NUMBER_FLOAT64"
#endif /* !CONFIG_ECMA_NUMBER_FLOAT32 && !CONFIG_ECMA_NUMBER_FLOAT64 */

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
#ifdef CONFIG_ECMA_NUMBER_FLOAT32
# define ECMA_NUMBER_MIN_VALUE (FLT_MIN)
# define ECMA_NUMBER_MAX_VALUE (FLT_MAX)
#elif defined (CONFIG_ECMA_NUMBER_FLOAT64)
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
#else /* !CONFIG_ECMA_NUMBER_FLOAT32 && !CONFIG_ECMA_NUMBER_FLOAT64 */
# error "!CONFIG_ECMA_NUMBER_FLOAT32 && !CONFIG_ECMA_NUMBER_FLOAT64"
#endif /* !CONFIG_ECMA_NUMBER_FLOAT32 && !CONFIG_ECMA_NUMBER_FLOAT64 */


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
 * Null character (zt-string end marker)
 */
#define ECMA_CHAR_NULL  ((ecma_char_t) '\0')

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
 * Description of a collection's/string's length
 */
typedef uint16_t ecma_length_t;

/**
 * Description of a collection's header.
 */
typedef struct
{
  /** Compressed pointer to next chunk with collection's data */
  uint16_t next_chunk_cp;

  /** Number of elements in the collection */
  ecma_length_t unit_number;

  /** Place for the collection's data */
  uint8_t data[ sizeof (uint64_t) - sizeof (uint32_t) ];
} ecma_collection_header_t;

/**
 * Description of non-first chunk in a collection's chain of chunks
 */
typedef struct
{
  /** Compressed pointer to next chunk */
  uint16_t next_chunk_cp;

  /** Characters */
  uint8_t data[ sizeof (uint64_t) - sizeof (uint16_t) ];
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
  ECMA_STRING_CONTAINER_CHARS_IN_DESC, /**< actual data are several characters
                                            stored locally in the string's descriptor */
  ECMA_STRING_CONTAINER_UINT32_IN_DESC, /**< actual data is UInt32-represeneted Number
                                             stored locally in the string's descriptor */
  ECMA_STRING_CONTAINER_CONCATENATION, /**< the ecma-string is concatenation of two specified ecma-strings */
  ECMA_STRING_CONTAINER_MAGIC_STRING /**< the ecma-string is equal to one of ECMA magic strings */
} ecma_string_container_t;

FIXME (Move to library that should define the type (libserializer /* ? */))
/**
 * Index in literal table
 */
typedef uint32_t literal_index_t;

/**
 * Identifiers of ECMA magic string constants
 */
typedef enum
{
  ECMA_MAGIC_STRING_ARGUMENTS, /**< "arguments" */
  ECMA_MAGIC_STRING_EVAL, /**< "eval" */
  ECMA_MAGIC_STRING_PROTOTYPE, /**< "prototype" */
  ECMA_MAGIC_STRING_CONSTRUCTOR, /**< "constructor" */
  ECMA_MAGIC_STRING_CALLER, /**< "caller" */
  ECMA_MAGIC_STRING_CALLEE, /**< "callee" */
  ECMA_MAGIC_STRING_UNDEFINED, /**< "undefined" */
  ECMA_MAGIC_STRING_NULL, /**< "null" */
  ECMA_MAGIC_STRING_FALSE, /**< "false" */
  ECMA_MAGIC_STRING_TRUE, /**< "true" */
  ECMA_MAGIC_STRING_BOOLEAN, /**< "boolean" */
  ECMA_MAGIC_STRING_NUMBER, /**< "number" */
  ECMA_MAGIC_STRING_STRING, /**< "string" */
  ECMA_MAGIC_STRING_OBJECT, /**< "object" */
  ECMA_MAGIC_STRING_FUNCTION, /**< "function" */
  ECMA_MAGIC_STRING_LENGTH, /**< "length" */
  ECMA_MAGIC_STRING_NAN, /**< "NaN" */
  ECMA_MAGIC_STRING_INFINITY_UL, /**< "Infinity" */
  ECMA_MAGIC_STRING_UNDEFINED_UL, /**< "Undefined" */
  ECMA_MAGIC_STRING_NULL_UL, /**< "Null" */
  ECMA_MAGIC_STRING_OBJECT_UL, /**< "Object" */
  ECMA_MAGIC_STRING_FUNCTION_UL, /**< "Function" */
  ECMA_MAGIC_STRING_ARRAY_UL, /**< "Array" */
  ECMA_MAGIC_STRING_ARGUMENTS_UL, /**< "Arguments" */
  ECMA_MAGIC_STRING_STRING_UL, /**< "String" */
  ECMA_MAGIC_STRING_BOOLEAN_UL, /**< "Boolean" */
  ECMA_MAGIC_STRING_NUMBER_UL, /**< "Number" */
  ECMA_MAGIC_STRING_DATE_UL, /**< "Date" */
  ECMA_MAGIC_STRING_REG_EXP_UL, /**< "RegExp" */
  ECMA_MAGIC_STRING_ERROR_UL, /**< "Error" */
  ECMA_MAGIC_STRING_EVAL_ERROR_UL, /**< "EvalError" */
  ECMA_MAGIC_STRING_RANGE_ERROR_UL, /**< "RangeError" */
  ECMA_MAGIC_STRING_REFERENCE_ERROR_UL, /**< "ReferenceError" */
  ECMA_MAGIC_STRING_SYNTAX_ERROR_UL, /**< "SyntaxError" */
  ECMA_MAGIC_STRING_TYPE_ERROR_UL, /**< "TypeError" */
  ECMA_MAGIC_STRING_URI_ERROR_UL, /**< "URIError" */
  ECMA_MAGIC_STRING_MATH_UL, /**< "Math" */
  ECMA_MAGIC_STRING_JSON_U, /**< "JSON" */
  ECMA_MAGIC_STRING_PARSE_INT, /**< "parseInt" */
  ECMA_MAGIC_STRING_PARSE_FLOAT, /**< "parseFloat" */
  ECMA_MAGIC_STRING_IS_NAN, /**< "isNaN" */
  ECMA_MAGIC_STRING_IS_FINITE, /**< "isFinite" */
  ECMA_MAGIC_STRING_DECODE_URI, /**< "decodeURI" */
  ECMA_MAGIC_STRING_DECODE_URI_COMPONENT, /**< "decodeURIComponent" */
  ECMA_MAGIC_STRING_ENCODE_URI, /**< "encodeURI" */
  ECMA_MAGIC_STRING_ENCODE_URI_COMPONENT, /**< "encodeURIComponent" */
  ECMA_MAGIC_STRING_GET_PROTOTYPE_OF_UL, /**< "getPrototypeOf" */
  ECMA_MAGIC_STRING_GET_OWN_PROPERTY_DESCRIPTOR_UL, /**< "getOwnPropertyDescriptor" */
  ECMA_MAGIC_STRING_GET_OWN_PROPERTY_NAMES_UL, /**< "getOwnPropertyNames" */
  ECMA_MAGIC_STRING_CREATE, /**< "create" */
  ECMA_MAGIC_STRING_DEFINE_PROPERTY_UL, /**< "defineProperty" */
  ECMA_MAGIC_STRING_DEFINE_PROPERTIES_UL, /**< "defineProperties" */
  ECMA_MAGIC_STRING_SEAL, /**< "seal" */
  ECMA_MAGIC_STRING_FREEZE, /**< "freeze" */
  ECMA_MAGIC_STRING_PREVENT_EXTENSIONS_UL, /**< "preventExtensions" */
  ECMA_MAGIC_STRING_IS_SEALED_UL, /**< "isSealed" */
  ECMA_MAGIC_STRING_IS_FROZEN_UL, /**< "isFrozen" */
  ECMA_MAGIC_STRING_IS_EXTENSIBLE, /**< "isExtensible" */
  ECMA_MAGIC_STRING_KEYS, /**< "keys" */
  ECMA_MAGIC_STRING_WRITABLE, /**< "writable" */
  ECMA_MAGIC_STRING_ENUMERABLE, /**< "enumerable" */
  ECMA_MAGIC_STRING_CONFIGURABLE, /**< "configurable" */
  ECMA_MAGIC_STRING_VALUE, /**< "value" */
  ECMA_MAGIC_STRING_GET, /**< "get" */
  ECMA_MAGIC_STRING_SET, /**< "set" */
  ECMA_MAGIC_STRING_E_U, /**< "E" */
  ECMA_MAGIC_STRING_LN10_U, /**< "LN10" */
  ECMA_MAGIC_STRING_LN2_U, /**< "LN2" */
  ECMA_MAGIC_STRING_LOG2E_U, /**< "LOG2E" */
  ECMA_MAGIC_STRING_LOG10E_U, /**< "LOG10E" */
  ECMA_MAGIC_STRING_PI_U, /**< "PI" */
  ECMA_MAGIC_STRING_SQRT1_2_U, /**< "SQRT1_2" */
  ECMA_MAGIC_STRING_SQRT2_U, /**< "SQRT2" */
  ECMA_MAGIC_STRING_ABS, /**< "abs" */
  ECMA_MAGIC_STRING_ACOS, /**< "acos" */
  ECMA_MAGIC_STRING_ASIN, /**< "asin" */
  ECMA_MAGIC_STRING_ATAN, /**< "atan" */
  ECMA_MAGIC_STRING_ATAN2, /**< "atan2" */
  ECMA_MAGIC_STRING_CEIL, /**< "ceil" */
  ECMA_MAGIC_STRING_COS, /**< "cos" */
  ECMA_MAGIC_STRING_EXP, /** "exp" */
  ECMA_MAGIC_STRING_FLOOR, /**< "floor" */
  ECMA_MAGIC_STRING_LOG, /**< "log" */
  ECMA_MAGIC_STRING_MAX, /**< "max" */
  ECMA_MAGIC_STRING_MIN, /**< "min" */
  ECMA_MAGIC_STRING_POW, /**< "pow" */
  ECMA_MAGIC_STRING_RANDOM, /**< "random" */
  ECMA_MAGIC_STRING_ROUND, /**< "round" */
  ECMA_MAGIC_STRING_SIN, /**< "sin" */
  ECMA_MAGIC_STRING_SQRT, /**< "sqrt" */
  ECMA_MAGIC_STRING_TAN, /**< "tan" */
  ECMA_MAGIC_STRING_FROM_CHAR_CODE_UL, /**< "fromCharCode" */
  ECMA_MAGIC_STRING_IS_ARRAY_UL, /**< "isArray" */
  ECMA_MAGIC_STRING_TO_STRING_UL, /**< "toString" */
  ECMA_MAGIC_STRING_VALUE_OF_UL, /**< "valueOf" */
  ECMA_MAGIC_STRING_TO_LOCALE_STRING_UL, /**< "toLocaleString" */
  ECMA_MAGIC_STRING_HAS_OWN_PROPERTY_UL, /**< "hasOwnProperty" */
  ECMA_MAGIC_STRING_IS_PROTOTYPE_OF_UL, /**< "isPrototypeOf" */
  ECMA_MAGIC_STRING_PROPERTY_IS_ENUMERABLE_UL, /**< "propertyIsEnumerable" */
  ECMA_MAGIC_STRING_CONCAT, /**< "concat" */
  ECMA_MAGIC_STRING_POP, /**< "pop" */
  ECMA_MAGIC_STRING_JOIN, /**< "join" */
  ECMA_MAGIC_STRING_PUSH, /**< "push" */
  ECMA_MAGIC_STRING_REVERSE, /**< "reverse" */
  ECMA_MAGIC_STRING_SHIFT, /**< "shift" */
  ECMA_MAGIC_STRING_SLICE, /**< "slice" */
  ECMA_MAGIC_STRING_SORT, /**< "sort" */
  ECMA_MAGIC_STRING_SPLICE, /**< "splice" */
  ECMA_MAGIC_STRING_UNSHIFT, /**< "unshift" */
  ECMA_MAGIC_STRING_INDEX_OF_UL, /**< "indexOf" */
  ECMA_MAGIC_STRING_LAST_INDEX_OF_UL, /**< "lastIndexOf" */
  ECMA_MAGIC_STRING_EVERY, /**< "every" */
  ECMA_MAGIC_STRING_SOME, /**< "some" */
  ECMA_MAGIC_STRING_FOR_EACH_UL, /**< "forEach" */
  ECMA_MAGIC_STRING_MAP, /**< "map" */
  ECMA_MAGIC_STRING_FILTER, /**< "filter" */
  ECMA_MAGIC_STRING_REDUCE, /**< "reduce" */
  ECMA_MAGIC_STRING_REDUCE_RIGHT_UL, /**< "reduceRight" */
  ECMA_MAGIC_STRING_CHAR_AT_UL, /**< "charAt" */
  ECMA_MAGIC_STRING_CHAR_CODE_AT_UL, /**< "charCodeAt" */
  ECMA_MAGIC_STRING_LOCALE_COMPARE_UL, /**< "localeCompare" */
  ECMA_MAGIC_STRING_MATCH, /**< "match" */
  ECMA_MAGIC_STRING_REPLACE, /**< "replace" */
  ECMA_MAGIC_STRING_SEARCH, /**< "search" */
  ECMA_MAGIC_STRING_SPLIT, /**< "split" */
  ECMA_MAGIC_STRING_SUBSTRING, /**< "substring" */
  ECMA_MAGIC_STRING_TO_LOWER_CASE_UL, /**< "toLowerCase" */
  ECMA_MAGIC_STRING_TO_LOCALE_LOWER_CASE_UL, /**< "toLocaleLowerCase" */
  ECMA_MAGIC_STRING_TO_UPPER_CASE_UL, /**< "toUpperCase" */
  ECMA_MAGIC_STRING_TO_LOCALE_UPPER_CASE_UL, /**< "toLocaleUpperCase" */
  ECMA_MAGIC_STRING_TRIM, /**< "trim" */
  ECMA_MAGIC_STRING_TO_FIXED_UL, /**< "toFixed" */
  ECMA_MAGIC_STRING_TO_EXPONENTIAL_UL, /**< "toExponential" */
  ECMA_MAGIC_STRING_TO_PRECISION_UL, /**< "toPrecision" */
  ECMA_MAGIC_STRING_TO_DATE_STRING_UL, /**< "toDateString" */
  ECMA_MAGIC_STRING_TO_TIME_STRING_UL, /**< "toTimeString" */
  ECMA_MAGIC_STRING_TO_LOCALE_DATE_STRING_UL, /**< "toLocaleDateString" */
  ECMA_MAGIC_STRING_TO_LOCALE_TIME_STRING_UL, /**< "toLocaleTimeString" */
  ECMA_MAGIC_STRING_GET_TIME_UL, /**< "getTime" */
  ECMA_MAGIC_STRING_GET_FULL_YEAR_UL, /**< "getFullYear" */
  ECMA_MAGIC_STRING_GET_UTC_FULL_YEAR_UL, /**< "getUTCFullYear" */
  ECMA_MAGIC_STRING_GET_MONTH_UL, /**< "getMonth" */
  ECMA_MAGIC_STRING_GET_UTC_MONTH_UL, /**< "getUTCMonth" */
  ECMA_MAGIC_STRING_GET_DATE_UL, /**< "getDate" */
  ECMA_MAGIC_STRING_GET_UTC_DATE_UL, /**< "getUTCDate" */
  ECMA_MAGIC_STRING_GET_DAY_UL, /**< "getDay" */
  ECMA_MAGIC_STRING_GET_UTC_DAY_UL, /**< "getUTCDay" */
  ECMA_MAGIC_STRING_GET_HOURS_UL, /**< "getHours" */
  ECMA_MAGIC_STRING_GET_UTC_HOURS_UL, /**< "getUTCHours" */
  ECMA_MAGIC_STRING_GET_MINUTES_UL, /**< "getMinutes" */
  ECMA_MAGIC_STRING_GET_UTC_MINUTES_UL, /**< "getUTCMinutes" */
  ECMA_MAGIC_STRING_GET_SECONDS_UL, /**< "getSeconds" */
  ECMA_MAGIC_STRING_GET_UTC_SECONDS_UL, /**< "getUTCSeconds" */
  ECMA_MAGIC_STRING_GET_MILLISECONDS_UL, /**< "getMilliseconds" */
  ECMA_MAGIC_STRING_GET_UTC_MILLISECONDS_UL, /**< "getUTCMilliseconds" */
  ECMA_MAGIC_STRING_GET_TIMEZONE_OFFSET_UL, /**< "getTimezoneOffset" */
  ECMA_MAGIC_STRING_SET_TIME_UL, /**< "setTime" */
  ECMA_MAGIC_STRING_SET_MILLISECONDS_UL, /**< "setMilliseconds" */
  ECMA_MAGIC_STRING_SET_UTC_MILLISECONDS_UL, /**< "setUTCMilliseconds" */
  ECMA_MAGIC_STRING_SET_SECONDS_UL, /**< "setSeconds" */
  ECMA_MAGIC_STRING_SET_UTC_SECONDS_UL, /**< "setUTCSeconds" */
  ECMA_MAGIC_STRING_SET_MINUTES_UL, /**< "setMinutes" */
  ECMA_MAGIC_STRING_SET_UTC_MINUTES_UL, /**< "setUTCMinutes" */
  ECMA_MAGIC_STRING_SET_HOURS_UL, /**< "setHours" */
  ECMA_MAGIC_STRING_SET_UTC_HOURS_UL, /**< "setUTCHours" */
  ECMA_MAGIC_STRING_SET_DATE_UL, /**< "setDate" */
  ECMA_MAGIC_STRING_SET_UTC_DATE_UL, /**< "setUTCDate" */
  ECMA_MAGIC_STRING_SET_MONTH_UL, /**< "setMonth" */
  ECMA_MAGIC_STRING_SET_UTC_MONTH_UL, /**< "setUTCMonth" */
  ECMA_MAGIC_STRING_SET_FULL_YEAR_UL, /**< "setFullYear" */
  ECMA_MAGIC_STRING_SET_UTC_FULL_YEAR_UL, /**< "setUTCFullYear" */
  ECMA_MAGIC_STRING_TO_UTC_STRING_UL, /**< "toUTCString" */
  ECMA_MAGIC_STRING_TO_ISO_STRING_UL, /**< "toISOString" */
  ECMA_MAGIC_STRING_TO_JSON_UL, /**< "toJSON" */
  ECMA_MAGIC_STRING_MAX_VALUE_U, /**< "MAX_VALUE" */
  ECMA_MAGIC_STRING_MIN_VALUE_U, /**< "MIN_VALUE" */
  ECMA_MAGIC_STRING_POSITIVE_INFINITY_U, /**< "POSITIVE_INFINITY" */
  ECMA_MAGIC_STRING_NEGATIVE_INFINITY_U, /**< "NEGATIVE_INFINITY" */
  ECMA_MAGIC_STRING_COMPACT_PROFILE_ERROR_UL, /**< "CompactProfileError" */
  ECMA_MAGIC_STRING_APPLY, /**< "apply" */
  ECMA_MAGIC_STRING_CALL, /**< "call" */
  ECMA_MAGIC_STRING_BIND, /**< "bind" */
  ECMA_MAGIC_STRING_EXEC, /**< "exec" */
  ECMA_MAGIC_STRING_TEST, /**< "test" */
  ECMA_MAGIC_STRING_NAME, /**< "name" */
  ECMA_MAGIC_STRING_MESSAGE, /**< "message" */
  ECMA_MAGIC_STRING_LEFT_SQUARE_CHAR, /**< "[" */
  ECMA_MAGIC_STRING_RIGHT_SQUARE_CHAR, /**< "]" */
  ECMA_MAGIC_STRING_SPACE_CHAR, /**< " " */
  ECMA_MAGIC_STRING__EMPTY, /**< "" */
  ECMA_MAGIC_STRING__COUNT /**< number of magic strings */
} ecma_magic_string_id_t;

/**
 * ECMA string-value descriptor
 */
typedef struct
{
  /** Reference counter for the string */
  unsigned int refs : CONFIG_ECMA_REFERENCE_COUNTER_WIDTH;

  /** Where the string's data is placed (ecma_string_container_t) */
  unsigned int container : 3;

  /** String's length */
  ecma_length_t length;

  /**
   * Actual data or identifier of it's place in container (depending on 'container' field)
   */
  union
  {
    /** Index of string in literal table */
    literal_index_t lit_index;

    /** Compressed pointer to an ecma_collection_chunk_t */
    unsigned int chunk_cp : ECMA_POINTER_FIELD_WIDTH;

    /** Compressed pointer to an ecma_number_t */
    unsigned int number_cp : ECMA_POINTER_FIELD_WIDTH;
    
    /** Actual data placed locally in the descriptor */
    ecma_char_t chars[ sizeof (uint32_t) ];

    /** UInt32-represented number placed locally in the descriptor */
    uint32_t uint32_number;

    /** Representation of concatenation */
    struct
    {
      unsigned int string1_cp : ECMA_POINTER_FIELD_WIDTH;
      unsigned int string2_cp : ECMA_POINTER_FIELD_WIDTH;
    } concatenation;

    /** Identifier of magic string */
    ecma_magic_string_id_t magic_string_id;
  } u;
} ecma_string_t;

/**
 * \addtogroup reference ECMA-reference
 * @{
 */

/**
 * ECMA-reference (see also: ECMA-262 v5, 8.7).
 */
typedef struct
{
  /** base value */
  ecma_value_t base;

  /** referenced name */
  ecma_string_t *referenced_name_p;

  /** strict reference flag */
  bool is_strict;
} ecma_reference_t;

/**
 * @}
 */

#endif  /* JERRY_ECMA_GLOBALS_H */

/**
 * @}
 * @}
 */
