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

#ifndef JERRY_ECMA_DEFS_H
#define	JERRY_ECMA_DEFS_H

#include "defs.h"
#include "mem_allocator.h"

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
} ecma_Type_t;

/**
 * Simple ecma-values
 */
typedef enum {
    ECMA_SIMPLE_VALUE_UNDEFINED, /**< undefined value */
    ECMA_SIMPLE_VALUE_NULL, /**< null value */
    ECMA_SIMPLE_VALUE_FALSE, /**< boolean false */
    ECMA_SIMPLE_VALUE_TRUE, /**< boolean true */
    ECMA_SIMPLE_VALUE__COUNT /** count of simple ecma-values */
} ecma_SimpleValue_t;

/**
 * Type of ecma-property
 */
typedef enum {
    ECMA_PROPERTY_NAMEDDATA, /**< named data property */
    ECMA_PROPERTY_NAMEDACCESSOR, /**< named accessor property */
    ECMA_PROPERTY_INTERNAL /**< internal property */
} ecma_PropertyType_t;

/**
 * Description of an ecma-value
 */
typedef struct {
    /** Value type (ecma_Type_t) */
    uint32_t m_ValueType : 2;

    /**
     * Simple value (ecma_SimpleValue_t) or compressed pointer to value (depending on m_ValueType)
     */
    uint32_t m_Value : ECMA_POINTER_FIELD_WIDTH;
} __packed ecma_Value_t;

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
} ecma_InternalPropertyId_t;

/**
 * Description of ecma-property.
 */
typedef struct ecma_Property_t {
    /** Property's type (ecma_PropertyType_t) */
    uint32_t m_Type : 2;

    /** Compressed pointer to next property */
    uint32_t m_pNextProperty : ECMA_POINTER_FIELD_WIDTH;

    /** Property's details (depending on m_Type) */
    union {

        /** Description of named data property */
        struct __packed ecma_NamedDataProperty_t {
            /** Compressed pointer to property's name (pointer to String) */
            uint32_t m_pName : ECMA_POINTER_FIELD_WIDTH;

            /** Attribute 'Writable' */
            uint32_t m_Writable : 1;

            /** Attribute 'Enumerable' */
            uint32_t m_Enumerable : 1;

            /** Attribute 'Configurable' */
            uint32_t m_Configurable : 1;

            /** Value */
            ecma_Value_t m_Value;
        } m_NamedDataProperty;

        /** Description of named accessor property */
        struct __packed ecma_NamedAccessorProperty_t {
            /** Compressed pointer to property's name (pointer to String) */
            uint32_t m_pName : ECMA_POINTER_FIELD_WIDTH;

            /** Attribute 'Enumerable' */
            uint32_t m_Enumerable : 1;

            /** Attribute 'Configurable' */
            uint32_t m_Configurable : 1;

            /** Compressed pointer to property's getter */
            uint32_t m_pGet : ECMA_POINTER_FIELD_WIDTH;

            /** Compressed pointer to property's setter */
            uint32_t m_pSet : ECMA_POINTER_FIELD_WIDTH;
        } m_NamedAccessorProperty;

        /** Description of internal property */
        struct __packed ecma_InternalProperty_t {
            /** Internal property's type */
            uint32_t m_InternalPropertyType : 4;

            /** Value (may be a compressed pointer) */
            uint32_t m_Value : ECMA_POINTER_FIELD_WIDTH;
        } m_InternalProperty;
    } u;
} ecma_Property_t;

/**
 * Description of GC's information layout
 */
typedef struct {
    /**
     * Flag that indicates if the object is valid for normal usage.
     * If the flag is zero, then the object is not valid and is queued for GC.
     */
    uint32_t m_IsObjectValid : 1;

    /** Details (depending on m_IsObjectValid) */
    union {
        /**
         * Number of refs to the object (if m_IsObjectValid).
         * 
         * Note: It is not a pointer. Maximum value of reference counter
         *       willn't be bigger than overall count of variables/objects/properties,
         *       which is limited by size of address space allocated for JerryScript
         *       (and, consequently, by ECMA_POINTER_FIELD_WIDTH).
         */
        uint32_t m_Refs : ECMA_POINTER_FIELD_WIDTH;

        /** Compressed pointer to next object in the list of objects, queued for GC (if !m_IsObjectValid) */
        uint32_t m_NextQueuedForGC : ECMA_POINTER_FIELD_WIDTH;
    } __packed u;
} ecma_GCInfo_t;

/**
 * Types of lexical environments
 */
typedef enum {
    ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE, /**< declarative lexical environment */
    ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND /**< object-bound lexical environment */
} ecma_LexicalEnvironmentType_t;

/**
 * Description of ECMA-object or lexical environment
 * (depending on m_IsLexicalEnvironment).
 */
typedef struct ecma_Object_t {
    /** Compressed pointer to property list */
    uint32_t m_pProperties : ECMA_POINTER_FIELD_WIDTH;

    /** Flag indicating whether it is a general object (false)
        or a lexical environment (true) */
    uint32_t m_IsLexicalEnvironment : 1;

    /**
     * Attributes of either general object or lexical environment
     * (depending on m_IsLexicalEnvironment)
     */
    union {
        /**
         * A general object's attributes (if !m_IsLexicalEnvironment)
         */
        struct {
            /** Attribute 'Extensible' */
            uint32_t m_Extensible : 1;

            /** Compressed pointer to prototype object (ecma_Object_t) */
            uint32_t m_pPrototypeObject : ECMA_POINTER_FIELD_WIDTH;
        } __packed m_Object;

        /**
         * A lexical environment's attribute (if m_IsLexicalEnvironment)
         */
        struct {
            /**
             * Type of lexical environment (ecma_LexicalEnvironmentType_t).
             */
            uint32_t m_Type : 1;

            /** Compressed pointer to outer lexical environment */
            uint32_t m_pOuterReference : ECMA_POINTER_FIELD_WIDTH;
        } __packed m_LexicalEnvironment;

    } __packed u_Attributes;

    /** GC's information */
    ecma_GCInfo_t m_GCInfo;
} ecma_Object_t;

/**
 * Description of an ecma-character
 */
typedef uint16_t ecma_Char_t;

/**
 * Description of an ecma-number
 */
typedef double ecma_Number_t;

/**
 * Description of arrays'/strings' length
 */
typedef uint16_t ecma_Length_t;

/**
 * Description of an Array's header
 */
typedef struct {
    /** Compressed pointer to next chunk */
    uint16_t m_pNextChunk;

    /** Number of elements in the Array */
    uint16_t m_UnitNumber;
} ecma_ArrayHeader_t;

/**
 * Size of a chunk, containing a String's part, in bytes
 */
#define ECMA_ARRAY_CHUNK_SIZE_IN_BYTES  32

/**
 * Description of first chunk in a chain of chunks that contains an Array.
 */
typedef struct {
    /** Array's header */
    ecma_ArrayHeader_t m_Header;

    /** Elements */
    uint8_t m_Elements[ ECMA_ARRAY_CHUNK_SIZE_IN_BYTES - sizeof (ecma_ArrayHeader_t) ];
} ecma_ArrayFirstChunk_t;

/**
 * Description of non-first chunk in a chain of chunks that contains an Array
 */
typedef struct {
    /** Compressed pointer to next chunk */
    uint16_t m_pNextChunk;

    /** Characters */
    uint8_t m_Elements[ ECMA_ARRAY_CHUNK_SIZE_IN_BYTES - sizeof (uint16_t) ];
} ecma_ArrayNonFirstChunk_t;

#endif	/* JERRY_ECMA_DEFS_H */

/**
 * @}
 * @}
 */
