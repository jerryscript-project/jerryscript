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

/** \addtogroup ecma ---TODO---
 * @{
 *
 * \addtogroup ecmatypes ECMA types
 * @{
 */

#ifndef JERRY_ECMA_GLOBALS_H
#define	JERRY_ECMA_GLOBALS_H

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
#define ECMA_POINTER_FIELD_WIDTH 14

/**
 * The null value for compressed pointers
 */
#define ECMA_NULL_POINTER 0

/**
 * @}
 */

/**
 * Type of ecma-value
 */
typedef enum {
    ECMA_TYPE_SIMPLE, /**< simple value */
    ECMA_TYPE_NUMBER, /**< 64-bit integer */
    ECMA_TYPE_STRING, /**< pointer to description of a string */
    ECMA_TYPE_OBJECT, /**< pointer to description of an object */
    ECMA_TYPE__COUNT /**< count of types */
} ecma_type_t;

/**
 * Simple ecma-values
 */
typedef enum {
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
    ECMA_SIMPLE_VALUE_ARRAY_REDIRECT, /**< implementation defined value for an array's elements that exists,
                                           but is stored directly in the array's property list
                                           (used for array elements with non-default attribute values) */
    ECMA_SIMPLE_VALUE__COUNT /** count of simple ecma-values */
} ecma_simple_value_t;

/**
 * Type of ecma-property
 */
typedef enum {
    ECMA_PROPERTY_NAMEDDATA, /**< named data property */
    ECMA_PROPERTY_NAMEDACCESSOR, /**< named accessor property */
    ECMA_PROPERTY_INTERNAL /**< internal property */
} ecma_property_type_t;

/**
 * Type of block evaluation (completion) result.
 *
 * See also: ECMA-262 v5, 8.9.
 */
typedef enum {
    ECMA_COMPLETION_TYPE_NORMAL, /**< default block completion */
    ECMA_COMPLETION_TYPE_RETURN, /**< block completed with return */
    ECMA_COMPLETION_TYPE_BREAK, /**< block completed with break */
    ECMA_COMPLETION_TYPE_CONTINUE, /**< block completed with continue */
    ECMA_COMPLETION_TYPE_THROW, /**< block completed with throw */
    ECMA_COMPLETION_TYPE_EXIT /**< implementation-defined completion type
                                   for finishing script execution */
} ecma_completion_type_t;

/**
 * Description of an ecma-value
 */
typedef struct {
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
typedef struct {
    /** Type (ecma_completion_type_t) */
    unsigned int type : 3;

    /** Value */
    ecma_value_t value;

    /** Target */
    unsigned int target : 8;
} __packed ecma_completion_value_t;

/**
 * Target value indicating that target field
 * of ecma_completion_value_t defines no target.
 */
#define ECMA_TARGET_ID_RESERVED 255

/**
 * Internal properties' identifiers.
 */
typedef enum {
    ECMA_INTERNAL_PROPERTY_CLASS, /**< [[Class]] */
    ECMA_INTERNAL_PROPERTY_PROTOTYPE, /**< [[Prototype]] */
    ECMA_INTERNAL_PROPERTY_EXTENSIBLE, /**< [[Extensible]] */
    ECMA_INTERNAL_PROPERTY_SCOPE, /**< [[Scope]] */

    /** provideThis property of lexical environment */
    ECMA_INTERNAL_PROPERTY_PROVIDE_THIS,

    /** binding object of lexical environment */
    ECMA_INTERNAL_PROPERTY_BINDING_OBJECT,

    /** Part of an array, that is indexed by numbers */
    ECMA_INTERNAL_PROPERTY_NUMBER_INDEXED_ARRAY_VALUES,

    /** Part of an array, that is indexed by strings */
    ECMA_INTERNAL_PROPERTY_STRING_INDEXED_ARRAY_VALUES
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
 * Description of ecma-property.
 */
typedef struct ecma_property_t {
    /** Property's type (ecma_property_type_t) */
    unsigned int type : 2;

    /** Compressed pointer to next property */
    unsigned int next_property_p : ECMA_POINTER_FIELD_WIDTH;

    /** Property's details (depending on Type) */
    union {

        /** Description of named data property */
        struct __packed ecma_named_data_property_t {
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
        struct __packed ecma_named_accessor_property_t {
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
        struct __packed ecma_internal_property_t {
            /** Internal property's type */
            unsigned int internal_property_type : 4;

            /** Value (may be a compressed pointer) */
            unsigned int value : ECMA_POINTER_FIELD_WIDTH;
        } internal_property;
    } u;
} ecma_property_t;

/**
 * Description of GC's information layout
 */
typedef struct {
    FIXME( /* Handle cyclic dependencies */ )

    /**
     * Flag that indicates if the object is valid for normal usage.
     * If the flag is zero, then the object is not valid and is queued for GC.
     */
    unsigned int is_object_valid : 1;

    /** Details (depending on is_object_valid) */
    union {
        /**
         * Number of refs to the object (if is_object_valid).
         * 
         * Note: It is not a pointer. Maximum value of reference counter
         *       willn't be bigger than overall count of variables/objects/properties.
         *       The width of the field will be sufficient in most cases. However, it is not theoretically
         *       guaranteed. Overflow is handled in ecma_ref_object by stopping the engine.
         */
        unsigned int refs : ECMA_POINTER_FIELD_WIDTH;

        /** Compressed pointer to next object in the list of objects, queued for GC (if !is_object_valid) */
        unsigned int next_queued_for_gc : ECMA_POINTER_FIELD_WIDTH;
    } __packed u;
} __packed ecma_gc_info_t;

/**
 * Types of lexical environments
 */
typedef enum {
    ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE, /**< declarative lexical environment */
    ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND /**< object-bound lexical environment */
} ecma_lexical_environment_type_t;

/**
 * Internal object types
 */
typedef enum {
    ECMA_GENERAL_OBJECT, /**< all objects that are not String (15.5), Function (15.3),
                              Arguments (10.6), Array (15.4) specification-defined objects
                              and not host objects */
    ECMA_STRING_OBJECT, /**< String objects (15.5) */
    ECMA_FUNCTION_OBJECT, /**< Function objects (15.3) */
    ECMA_ARGUMENTS_OBJECT, /**< Arguments object (10.6) */
    ECMA_ARRAY_OBJECT, /**< Array object (15.4) */
    ECMA_HOST_OBJECT /**< Host object */
} ecma_object_type_t;

/**
 * Description of ECMA-object or lexical environment
 * (depending on is_lexical_environment).
 */
typedef struct ecma_object_t {
    /** Compressed pointer to property list */
    unsigned int properties_p : ECMA_POINTER_FIELD_WIDTH;

    /** Flag indicating whether it is a general object (false)
        or a lexical environment (true) */
    unsigned int is_lexical_environment : 1;

    /**
     * Attributes of either general object or lexical environment
     * (depending on is_lexical_environment)
     */
    union {
        /**
         * A general object's attributes (if !is_lexical_environment)
         */
        struct {
            /** Attribute 'Extensible' */
            unsigned int extensible : 1;

            /** Implementation internal object type (ecma_object_type_t) */
            unsigned int object_type : 3;

            /** Compressed pointer to prototype object (ecma_object_t) */
            unsigned int prototype_object_p : ECMA_POINTER_FIELD_WIDTH;
        } __packed object;

        /**
         * A lexical environment's attribute (if is_lexical_environment)
         */
        struct {
            /**
             * Type of lexical environment (ecma_lexical_environment_type_t).
             */
            unsigned int type : 1;

            /** Compressed pointer to outer lexical environment */
            unsigned int outer_reference_p : ECMA_POINTER_FIELD_WIDTH;
        } __packed lexical_environment;

    } __packed u;

    /** GC's information */
    ecma_gc_info_t GCInfo;
} __packed ecma_object_t;

/**
 * Description of an ecma-character
 */
typedef uint8_t ecma_char_t;

/**
 * Description of an ecma-number
 */
typedef float ecma_number_t;

/**
 * Value '0' of ecma_number_t
 */
#define ECMA_NUMBER_ZERO ((ecma_number_t) 0)

/**
 * Value '1' of ecma_number_t
 */
#define ECMA_NUMBER_ONE  ((ecma_number_t) 1)

/**
 * Description of arrays'/strings' length
 */
typedef uint16_t ecma_length_t;

/**
 * Description of an Array's header
 */
typedef struct {
    /** Compressed pointer to next chunk */
    uint16_t next_chunk_p;

    /** Number of elements in the Array */
    ecma_length_t unit_number;
} ecma_array_header_t;

/**
 * Size of a chunk, containing a String's part, in bytes
 */
#define ECMA_ARRAY_CHUNK_SIZE_IN_BYTES  32

/**
 * Description of first chunk in a chain of chunks that contains an Array.
 */
typedef struct {
    /** Array's header */
    ecma_array_header_t header;

    /** Elements */
    uint8_t data[ ECMA_ARRAY_CHUNK_SIZE_IN_BYTES - sizeof (ecma_array_header_t) ];
} ecma_array_first_chunk_t;

/**
 * Description of non-first chunk in a chain of chunks that contains an Array
 */
typedef struct {
    /** Compressed pointer to next chunk */
    uint16_t next_chunk_p;

    /** Characters */
    uint8_t data[ ECMA_ARRAY_CHUNK_SIZE_IN_BYTES - sizeof (uint16_t) ];
} ecma_array_non_first_chunk_t;

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

  /** referenced name value pointer */
  ecma_char_t *referenced_name_p;

  /** strict reference flag */
  bool is_strict;
} ecma_reference_t;

/**
 * @}
 */

#endif	/* JERRY_ECMA_GLOBALS_H */

/**
 * @}
 * @}
 */
