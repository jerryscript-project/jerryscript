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

#ifndef CTX_REFERENCE_H
#define CTX_REFERENCE_H

#include "ctx-reference.h"
#include "globals.h"
#include "ecma-defs.h"

/** \addtogroup ctxman Context manager
 * @{
 */

/**
 * \addtogroup syntacticreference Textual reference to variable/property
 * @{
 */

/**
 * Syntactic (textual/unresolved) reference to a variable/object's property.
 */
typedef struct {
    /**
     * Flag indicating that this is reference to a property.
     * 
     * Note:
     *  m_PropertyName is valid only if m_IsPropertyReference is true.
     */
    uint32_t m_IsPropertyReference : 1;

    /**
     * Flag indicating that this reference is strict (see also: ECMA-262 v5, 8.7).
     */
    uint32_t m_StrictReference : 1;

    /**
     * Name of variable (Null-terminated string).
     */
    ecma_Char_t* m_Name;

    /**
     * Name of object's property (Null-terminated string).
     */
    ecma_Char_t* m_PropertyName;
} ctx_SyntacticReference_t;

/**
 * @}
 */

/**
 * \addtogroup resolvedreference Resolved reference type
 * @{
 */

/**
 * Description of resolved reference.
 * 
 * Implementation details:
 *  1. In contrast to Reference specification type the referenced name
 *     is not stored as string, but is resolved and stored as pointer
 *     to ecma-property.
 * 
 *     If the referenced element is deleted, the m_IsValid must be set to false.
 * 
 *  2. Is base is Boolean, String, Number, then it is converted to Object via
 *     ecma_ToObject and then is stored in the reference.
 * 
 * See also: ECMA-262 v5, 8.7.
 */
typedef struct
{
    /**
     * Flag indicating whether the reference is valid.
     * 
     * The flag is initially set to true.
     * 
     */
    bool m_IsValid;

    /**
     * Base value
     * 
     * May be undefined (NULL), Object or Lexical Environment
     */
    ecma_Object_t* m_Base;
    
    /**
     * Referenced property.
     * 
     * Note:
     *      in case base is lexical environment this is reference to variable.
     */
    ecma_Property_t* m_ReferencedProperty;
    
    /**
     * Strict reference flag.
     */
    bool m_Strict;
} ctx_Reference_t;

/*
 * ctx-reference.c
 */
extern ecma_Object_t* ctx_reference_get_base( ctx_Reference_t *reference_p);
extern const ecma_ArrayFirstChunk_t* ctx_reference_get_referenced_name( ctx_Reference_t *reference_p);
extern bool ctx_reference_is_strict_reference( ctx_Reference_t *reference_p);
extern bool ctx_reference_is_property_reference( ctx_Reference_t *reference_p);
extern bool ctx_reference_is_unresolvable_reference( ctx_Reference_t *reference_p);
extern ecma_Property_t *ctx_reference_get_referenced_component( ctx_Reference_t *reference_p);

extern ctx_Reference_t* ctx_resolve_syntactic_reference( ecma_Object_t *lex_env_p, ctx_SyntacticReference_t *syntactic_reference_p);
extern void ctx_free_resolved_reference( ctx_Reference_t *reference_p);

/**
 * @}
 * @}
 */

#endif /* !CTX_REFERENCE_H */