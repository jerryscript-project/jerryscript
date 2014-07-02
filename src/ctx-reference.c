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

/** \addtogroup ctxman Context manager
 * @{
 *
 * \addtogroup resolvedreference Resolved reference type
 * @{
 */

/**
 * Implementation of Reference's operations
 */

#include "globals.h"
#include "ecma-defs.h"
#include "ecma-helpers.h"
#include "ctx-reference.h"

/**
 * GetBase operation of Reference.
 * 
 * @return base value component of reference
 */
ecma_Object_t*
ctx_reference_get_base( ctx_Reference_t *reference_p) /**< reference */
{
  return reference_p->m_Base;
} /* ctx_reference_get_base */

/**
 * GetReferencedName operation of Reference.
 * 
 * @return pointer to first chunk of ecma-array containing the referenced name
 */
const ecma_ArrayFirstChunk_t*
ctx_reference_get_referenced_name( ctx_Reference_t *reference_p) /**< reference */
{
  const ecma_Property_t *property_p = reference_p->m_ReferencedProperty;

  switch ( (ecma_PropertyType_t) property_p->m_Type )
    {
    case ECMA_PROPERTY_NAMEDDATA:
      return ecma_GetPointer( property_p->u.m_NamedDataProperty.m_pName);

    case ECMA_PROPERTY_NAMEDACCESSOR:
      return ecma_GetPointer( property_p->u.m_NamedAccessorProperty.m_pName);

    case ECMA_PROPERTY_INTERNAL:
      /* will trap below */
      break;
    }

  JERRY_UNREACHABLE();
} /* ctx_reference_get_referenced_name */

/**
 * IsStrictReference operation of Reference.
 * 
 * @return strict component of reference:
 *          true - if reference is strict,
 *          false - otherwise.
 */
bool
ctx_reference_is_strict_reference( ctx_Reference_t *reference_p) /**< reference */
{
  return reference_p->m_Strict;
} /* ctx_reference_is_strict_reference */

/**
 * IsPropertyReference operation of Reference.
 * 
 * @return true - if either the base value is an object or HasPrimitiveBase returns true;
 *         false - otherwise.
 */
bool
ctx_reference_is_property_reference( ctx_Reference_t * reference_p) /**< reference */
{

  return (reference_p->m_Base != NULL
          && !reference_p->m_Base->m_IsLexicalEnvironment );
} /* ctx_reference_is_property_reference */

/**
 * IsUnresolvableReference operation of Reference.
 * 
 * @return true - if the base value is undefined;
 *         false - otherwise.
 */
bool
ctx_reference_is_unresolvable_reference( ctx_Reference_t * reference_p) /**< reference */
{
  return ( reference_p->m_Base == NULL );
} /* ctx_reference_is_unresolvable_reference */

/**
 * Get referenced property.
 *
 * @return pointer to ecma-property
 *         (which describes object's property or a lexical environment's binding).
 */
ecma_Property_t*
ctx_reference_get_referenced_component( ctx_Reference_t *reference_p) /**< reference */
{
  return reference_p->m_ReferencedProperty;
} /* ctx_reference_get_referenced_component */

/**
 * Resolve syntactic reference
 *
 * Note:
 *      Returned value must be freed using ctx_free_resolved_reference
 * 
 * @return pointer to resolved reference description
 */
ctx_Reference_t*
ctx_resolve_syntactic_reference(ecma_Object_t *lex_env_p, /**< lexical environment of current context */
                                ctx_SyntacticReference_t *syntactic_reference_p) /** syntactic reference
                                                                                  *  to resolve */
{
  JERRY_ASSERT(lex_env_p != NULL
               && lex_env_p->m_GCInfo.m_IsObjectValid
               && lex_env_p->m_IsLexicalEnvironment );
  JERRY_ASSERT(syntactic_reference_p != NULL
               && syntactic_reference_p->m_Name != NULL
               && ( !syntactic_reference_p->m_IsPropertyReference
                   || syntactic_reference_p->m_PropertyName != NULL ) );

  ctx_Reference_t *reference_p = (ctx_Reference_t*) mem_HeapAllocBlock(sizeof (ctx_Reference_t), MEM_HEAP_ALLOC_LONG_TERM);

  bool is_variable_resolved = false;
  ecma_Property_t *resolved_variable_p = NULL;

  /* resolving variable name */
  while ( !is_variable_resolved && lex_env_p != NULL )
    {
      for ( ecma_Property_t *property_p = ecma_GetPointer( lex_env_p->m_pProperties);
           property_p != NULL;
           property_p = ecma_GetPointer( property_p->m_pNextProperty) )
        {
          ecma_ArrayFirstChunk_t *property_name_p = NULL;

          /*
           * TODO: make corresponding helper
           */
          switch ( (ecma_PropertyType_t) property_p->m_Type )
            {
            case ECMA_PROPERTY_NAMEDDATA:
              property_name_p = ecma_GetPointer( property_p->u.m_NamedDataProperty.m_pName);
              break;

            case ECMA_PROPERTY_NAMEDACCESSOR:
              property_name_p = ecma_GetPointer( property_p->u.m_NamedAccessorProperty.m_pName);
              break;

            case ECMA_PROPERTY_INTERNAL:
              continue;
            }

          if ( ecma_CompareCharBufferToEcmaString(syntactic_reference_p->m_Name,
                                                  property_name_p) )
            {
              resolved_variable_p = property_p;
              is_variable_resolved = true;

              break;
            }
        }

      lex_env_p = ecma_GetPointer( lex_env_p->u_Attributes.m_LexicalEnvironment.m_pOuterReference);
    }

  if ( !is_variable_resolved )
    {
      *reference_p = (ctx_Reference_t){
          .m_IsValid = true,
            .m_Base = NULL,
            .m_ReferencedProperty = NULL,
            .m_Strict = syntactic_reference_p->m_StrictReference
      };
    } else
      {
        if ( !syntactic_reference_p->m_IsPropertyReference )
          {
            *reference_p = (ctx_Reference_t){
                .m_IsValid = true,
                  .m_Base = lex_env_p,
                  .m_ReferencedProperty = resolved_variable_p,
                  .m_Strict = syntactic_reference_p->m_StrictReference
            };

          } else
            {
              JERRY_UNIMPLEMENTED();
            }
      }

    return reference_p;
} /* ctx_resolve_syntactic_reference */

void
ctx_free_resolved_reference( ctx_Reference_t *reference_p)
{
  (void)reference_p;

  JERRY_UNIMPLEMENTED();
} /* ctx_free_resolved_reference */

/**
 * @}
 * @}
 */
