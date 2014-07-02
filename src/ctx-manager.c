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

#include "ctx-manager.h"
#include "ctx-reference.h"
#include "globals.h"
#include "ecma-alloc.h"
#include "ecma-globals.h"
#include "ecma-conversion.h"
#include "ecma-gc.h"
#include "ecma-helpers.h"
#include "jerry-libc.h"
#include "mem-poolman.h"

/** \addtogroup ctxman Context manager
 * @{
 */

/**
 * Maximum depth of varibles' context nestings stack.
 *  
 *  TODO: Move to configuration header.
 */
#define CTX_MAX_NUMBER_OF_VARIABLES_CONTEXTS 32

/**
 * JerryScript needs at least one variables' context nesting.
 */
JERRY_STATIC_ASSERT( CTX_MAX_NUMBER_OF_VARIABLES_CONTEXTS >= 1 );

/**
 * Description of a variables' context
 */
typedef struct
{
    /**
     * Pointer to object, associated with 'this' keyword.
     */
    ecma_Object_t *pThisBinding;

    /**
     * Chain of lexical environments
     */
    ecma_Object_t *pLexicalEnvironment;
} ctx_VariablesContext_t;

/**
 * Stack of variables' contexts.
 */
static ctx_VariablesContext_t ctx_Stack[ CTX_MAX_NUMBER_OF_VARIABLES_CONTEXTS ];

/**
 * Current nestings' stack depth.
 */
static size_t ctx_ContextsNumber = 0;

/**
 * Current variables' context (context on the top of stack)
 */
#define ctx_CurrentContext ( ctx_Stack[ ctx_ContextsNumber - 1 ])

/**
 * The global object
 */
ecma_Object_t* ctx_pGlobalObject;

/**
 * Get ecma-value from variable
 * 
 * @return value descriptor
 */
static ecma_Value_t
ctx_GetValueDescriptorFromVariable( ctx_SyntacticReference_t *pVar) /**< variable */
{
   /*
     * TODO:
     */
    (void)pVar;
    JERRY_UNIMPLEMENTED();
} /* ctx_GetValueDescriptorFromVariable */

/**
 * Get ecma-value from variable
 * 
 * @return value descriptor
 */
static void
ctx_SetValueDescriptorToVariable(ctx_SyntacticReference_t *pVar, /**< variable */
                                 ecma_Value_t value) /**< value descriptor */
{
    /*
     * TODO:
     */
    (void)pVar;
    (void)value;
    JERRY_UNIMPLEMENTED();
} /* ctx_SetValueDescriptorToVariable */

/**
 * Allocate a context.
 */
static void
ctx_AllocContext( void)
{
    JERRY_ASSERT( ctx_ContextsNumber < CTX_MAX_NUMBER_OF_VARIABLES_CONTEXTS );

    ctx_ContextsNumber++;
} /* ctx_AllocContext */

/**
 * Create new lexical environment using specified object as binding object,
 * setting provideThis to specified value.
 * The lexical environment is inherited from current context's lexical environment.
 */
static void
ctx_CreateLexicalEnvironmentFromObject(ecma_Object_t *pObject, /**< pointer to bindingObject */
                                       bool provideThis) /**< value of 'provideThis' attribute */
{
    ecma_Object_t *pNewLexicalEnvironment = ecma_CreateLexicalEnvironment(ctx_CurrentContext.pLexicalEnvironment,
                                                                          true);
    /* We don't change reference counter of ctx_CurrentContext.pLexicalEnvironment here,
       because we remove one reference from ctx_CurrentContext,
       and add one reference from pNewLexicalEnvironment */
    ctx_CurrentContext.pLexicalEnvironment = pNewLexicalEnvironment;

    ecma_Property_t *pProvideThisProperty = ecma_CreateInternalProperty( pNewLexicalEnvironment, ECMA_INTERNAL_PROPERTY_PROVIDE_THIS);
    pProvideThisProperty->u.m_InternalProperty.m_Value = provideThis;

    ecma_Property_t *pBindingObjectProperty = ecma_CreateInternalProperty( pNewLexicalEnvironment, ECMA_INTERNAL_PROPERTY_BINDING_OBJECT);

    ecma_RefObject( pObject);
    ecma_SetPointer( pBindingObjectProperty->u.m_InternalProperty.m_Value, pObject);
} /* ctx_CreateLexicalEnvironmentFromObject */

/**
 * Initialize the global object.
 */
static void
ctx_InitGlobalObject( void)
{
    ctx_pGlobalObject = ecma_CreateObject( NULL, true);
} /* ctx_InitGlobalObject */

/**
 *  \addtogroup interface Context manager's interface
 * @{
 */

/**
 * Initialize context manager and global execution context.
 */
void
ctx_Init(void)
{
    JERRY_ASSERT( ctx_ContextsNumber == 0 );

#ifndef JERRY_NDEBUG
    libc_memset( ctx_Stack, 0, sizeof (ctx_Stack));
#endif /* !JERRY_NDEBUG */

    ctx_InitGlobalObject();
    ctx_NewContextFromGlobalObject();
} /* ctx_Init */

/**
 * Create new variables' context using global object
 * for ThisBinding and lexical environment.
 */
void
ctx_NewContextFromGlobalObject(void)
{
    ctx_AllocContext();

    ecma_RefObject( ctx_pGlobalObject);
    ctx_CurrentContext.pThisBinding = ctx_pGlobalObject;

    ctx_CurrentContext.pLexicalEnvironment = NULL;
    ctx_CreateLexicalEnvironmentFromObject( ctx_pGlobalObject, false);

    JERRY_ASSERT( ctx_CurrentContext.pLexicalEnvironment != NULL );
} /* ctx_NewContextFromGlobalObject */

/**
 * Create new variables' context inheriting lexical environment from specified
 * function's [[Scope]], and setting ThisBinding from pThisVar parameter
 * (see also ECMA-262 5.1, 10.4.3).
 */
void
ctx_NewContextFromFunctionScope(ctx_SyntacticReference_t *pThisVar, /**< object for ThisBinding */
                                ctx_SyntacticReference_t *pFunctionVar) /**< Function object */
{
    ctx_AllocContext();

    ecma_Value_t thisArgValue = ctx_GetValueDescriptorFromVariable( pThisVar);
    ecma_Value_t functionArgValue = ctx_GetValueDescriptorFromVariable( pFunctionVar);

    ecma_Object_t *pThisBindingObject;
    if ( thisArgValue.m_ValueType == ECMA_TYPE_SIMPLE
         && ( thisArgValue.m_Value == ECMA_SIMPLE_VALUE_NULL
             || thisArgValue.m_Value == ECMA_SIMPLE_VALUE_UNDEFINED ) )
    {
        pThisBindingObject = ctx_pGlobalObject;
    } else
    {
        pThisBindingObject = ecma_ToObject( thisArgValue);
    }

    ecma_RefObject( pThisBindingObject);
    ctx_CurrentContext.pThisBinding = pThisBindingObject;

    JERRY_ASSERT( functionArgValue.m_ValueType == ECMA_TYPE_OBJECT );
    ecma_Object_t *pFunctionObject = ecma_GetPointer( functionArgValue.m_Value);

    ecma_Property_t *pScopeProperty = ecma_GetInternalProperty( pFunctionObject, ECMA_INTERNAL_PROPERTY_SCOPE);

    ecma_Object_t *pScopeObject = ecma_GetPointer( pScopeProperty->u.m_InternalProperty.m_Value);

    ecma_RefObject( pScopeObject);
    ecma_Object_t *pLexicalEnvironment = ecma_CreateLexicalEnvironment(pScopeObject, false);

    /* We don't change reference counter of ctx_CurrentContext.pLexicalEnvironment here,
       because we remove one reference from ctx_CurrentContext,
       and add one reference from pNewLexicalEnvironment */
    ctx_CurrentContext.pLexicalEnvironment = pLexicalEnvironment;
} /* ctx_NewContextFromFunctionScope */

/**
 * Create new lexical environment using specified object as binding object,
 * setting provideThis to specified value.
 * The lexical environment is inherited from current context's lexical environment.
 */
void
ctx_NewLexicalEnvironmentFromObject(ctx_SyntacticReference_t *pObjectVar, /**< binding object */
                                    bool provideThis) /**< 'provideThis' attribute */
{
    ecma_Object_t *pObject = ecma_ToObject( ctx_GetValueDescriptorFromVariable( pObjectVar));

    ctx_CreateLexicalEnvironmentFromObject( pObject, provideThis);
} /* ctx_NewLexicalEnvironmentFromObject */

/**
 * Exit from levelsToExit lexical environments (i.e. choose lexical environment
 * that is levelsToExit outward current lexical environment as new current context's
 * lexical environment).
 */
void
ctx_ExitLexicalEnvironments(uint32_t levelsToExit) /**< number of lexical environments
                                                    *   to exit from */
{
    JERRY_ASSERT( levelsToExit > 0 );

    for ( uint32_t count = 0;
          count < levelsToExit;
          count++ )
    {
        JERRY_ASSERT( ctx_CurrentContext.pLexicalEnvironment != NULL );

        ecma_Object_t *pOuterLexicalEnvironment = ecma_GetPointer( ctx_CurrentContext.pLexicalEnvironment->u_Attributes.m_LexicalEnvironment.m_pOuterReference);

        ecma_DerefObject( ctx_CurrentContext.pLexicalEnvironment);

        ctx_CurrentContext.pLexicalEnvironment = pOuterLexicalEnvironment;
    }

    JERRY_ASSERT( ctx_CurrentContext.pLexicalEnvironment != NULL );
} /* ctx_ExitLexicalEnvironments */

/**
 * Exit from levelsToExit variables' contexts (i.e. choose context
 * that is levelsToExit from current context as new current context).
 */
void
ctx_ExitContexts(uint32_t levelsToExit) /**< number of contexts to exit from */
{
    JERRY_ASSERT( levelsToExit > 0 );

    for ( uint32_t count = 0;
          count < levelsToExit;
          count++ )
    {
        JERRY_ASSERT( ctx_ContextsNumber > 0 );

        ecma_DerefObject( ctx_CurrentContext.pThisBinding);

        while ( ctx_CurrentContext.pLexicalEnvironment != NULL )
        {
            ecma_Object_t *pOuterLexicalEnvironment =
                    ecma_GetPointer(ctx_CurrentContext.pLexicalEnvironment->
                                    u_Attributes.m_LexicalEnvironment.
                                    m_pOuterReference);

            ecma_DerefObject( ctx_CurrentContext.pLexicalEnvironment);

            ctx_CurrentContext.pLexicalEnvironment = pOuterLexicalEnvironment;
        }

        ctx_ContextsNumber--;
    }

    JERRY_ASSERT( ctx_ContextsNumber > 0 );
} /* ctx_ExitContexts */

/**
 * Create new variable with undefined value in the current lexical environment.
 */
void
ctx_NewVariable( ctx_SyntacticReference_t *pVar) /**< variable id */
{
    ecma_Object_t *lexicalEnvironment = ctx_CurrentContext.pLexicalEnvironment;

    /*
     * TODO:
     */
    (void) pVar;
    JERRY_UNIMPLEMENTED();

    switch ( (ecma_LexicalEnvironmentType_t) lexicalEnvironment->u_Attributes.m_LexicalEnvironment.m_Type  )
    {
        case ECMA_LEXICAL_ENVIRONMENT_OBJECTBOUND:
        {
            ecma_Property_t *pBindingObjectProperty = ecma_FindInternalProperty(lexicalEnvironment,
                                                                                ECMA_INTERNAL_PROPERTY_BINDING_OBJECT);
            JERRY_ASSERT( pBindingObjectProperty != NULL );

            ecma_Object_t *pBindingObject = ecma_GetPointer( pBindingObjectProperty->u.m_InternalProperty.m_Value);
            JERRY_ASSERT( pBindingObject != NULL );

            break;
        }

        case ECMA_LEXICAL_ENVIRONMENT_DECLARATIVE:
        {
            break;
        }
    }
} /* ctx_NewVariable */

/**
 * Delete specified variable.
 */
void
ctx_DeleteVariable( ctx_SyntacticReference_t *pVar) /**< variable id */
{
    /*
     * TODO:
     */
    (void) pVar;
    JERRY_UNIMPLEMENTED();
} /* ctx_DeleteVariable */

/**
 * Copy variable's/property's/array's element's value.
 */
void
ctx_CopyVariable(ctx_SyntacticReference_t *pVarFrom, /**< source variable */
                 ctx_SyntacticReference_t *pVarTo)   /**< destination variable */
{
    ecma_Value_t sourceVariableValue = ctx_GetValueDescriptorFromVariable( pVarFrom);
    ecma_Value_t destinationVariableValue;

    destinationVariableValue.m_ValueType = sourceVariableValue.m_ValueType;
    switch ( (ecma_Type_t) sourceVariableValue.m_ValueType )
    {
        case ECMA_TYPE_SIMPLE:
        {
            destinationVariableValue.m_Value = sourceVariableValue.m_Value;
            break;
        }

        case ECMA_TYPE_NUMBER:
        {
            ecma_Number_t *pNumberCopy = ecma_AllocNumber();
            libc_memcpy( pNumberCopy,
                        ecma_GetPointer( sourceVariableValue.m_Value),
                        sizeof (ecma_Number_t));
            ecma_SetPointer( destinationVariableValue.m_Value, pNumberCopy);
            break;
        }

        case ECMA_TYPE_STRING:
        {
            ecma_SetPointer(destinationVariableValue.m_Value,
                            ecma_DuplicateEcmaString( ecma_GetPointer( sourceVariableValue.m_Value)));
            break;
        }

        case ECMA_TYPE_OBJECT:
        {
            ecma_RefObject( ecma_GetPointer( sourceVariableValue.m_Value));
            destinationVariableValue.m_Value = sourceVariableValue.m_Value;
            break;
        }

        case ECMA_TYPE__COUNT:
        {
            JERRY_UNREACHABLE();
        }
    }

    ctx_SetValueDescriptorToVariable( pVarTo, destinationVariableValue);
} /* ctx_CopyVariable */

/**
 * Get type of value of specified variable/property/array's element.
 */
ecma_Type_t
ctx_GetVariableType(ctx_SyntacticReference_t *pVar) /**< variable */
{
    ecma_Value_t variableValue = ctx_GetValueDescriptorFromVariable( pVar);

    return variableValue.m_ValueType;
} /* ctx_GetVariableType */

/**
 * Get specified variable's/property's/array's element's value.
 * 
 * @return number of bytes, actually copied to the buffer, if variable value was copied successfully;
 *         negative number, which is calculated as negation of buffer size, that is required
 *         to hold the variable's value (in case size of buffer is insuficcient).
 */
ssize_t
ctx_GetVariableValue(ctx_SyntacticReference_t *pVar, /**< variable */
                     uint8_t *pBuffer,       /**< buffer */
                     size_t bufferSize)      /**< size of buffer */
{
    ecma_Value_t variableValue = ctx_GetValueDescriptorFromVariable( pVar);

    switch ( (ecma_Type_t) variableValue.m_ValueType )
    {
        case ECMA_TYPE_SIMPLE:
        {
            if ( bufferSize < sizeof (ecma_SimpleValue_t) )
            {
                return -(ssize_t)sizeof (ecma_SimpleValue_t);
            } else
            {
                *(ecma_SimpleValue_t*) pBuffer = variableValue.m_Value;

                return sizeof (ecma_SimpleValue_t);
            }
            break;
        }

        case ECMA_TYPE_NUMBER:
        {
            if ( bufferSize < sizeof (ecma_Number_t) )
            {
                return -(ssize_t)sizeof (ecma_Number_t);
            } else
            {
                ecma_Number_t *pNumber = ecma_GetPointer(variableValue.m_Value);
                *(ecma_Number_t*) pBuffer = *pNumber;

                return sizeof (ecma_Number_t);
            }
            break;
        }

        case ECMA_TYPE_STRING:
        {
            ecma_ArrayFirstChunk_t *pStringFirstChunk = ecma_GetPointer(variableValue.m_Value);

            return ecma_CopyEcmaStringCharsToBuffer( pStringFirstChunk, pBuffer, bufferSize);
        }

        case ECMA_TYPE_OBJECT: /* cannot return object itself (only value of a property or of an array's element */
        case ECMA_TYPE__COUNT:
        {
            /* will trap below */
        }
    }

    JERRY_UNREACHABLE();
} /* ctx_GetVariableValue */

/**
 * Set variable's/property's/array's element's value to one of simple values.
 */
void
ctx_SetVariableToSimpleValue(ctx_SyntacticReference_t *pVar, /**< variable */
                             ecma_SimpleValue_t value) /**< value */
{
    ecma_Value_t valueToSet;

    valueToSet.m_ValueType = ECMA_TYPE_SIMPLE;
    valueToSet.m_Value = value;

    ctx_SetValueDescriptorToVariable( pVar, valueToSet);
} /* ctx_SetVariableToSimpleValue */

/**
 * Set variable's/property's/array's element's value to a Number.
 */
void
ctx_SetVariableToNumber(ctx_SyntacticReference_t *pVar, /**< variable */
                        ecma_Number_t value) /**< value */
{
    ecma_Number_t *pNumber = ecma_AllocNumber();
    *pNumber = value;

    ecma_Value_t valueToSet;
    valueToSet.m_ValueType = ECMA_TYPE_NUMBER;
    ecma_SetPointer( valueToSet.m_Value, pNumber);

    ctx_SetValueDescriptorToVariable( pVar, valueToSet);
} /* ctx_SetVariableToNumber */

/**
 * Set variable's/property's/array's element's value to a String.
 */
void
ctx_SetVariableToString(ctx_SyntacticReference_t *pVar, /**< variable */
                        ecma_Char_t *value, /**< string's characters */
                        ecma_Length_t length) /**< string's length, in characters */
{
    ecma_Value_t valueToSet;
    valueToSet.m_ValueType = ECMA_TYPE_STRING;
    ecma_SetPointer( valueToSet.m_Value, ecma_NewEcmaString( value, length));

    ctx_SetValueDescriptorToVariable( pVar, valueToSet);
} /* ctx_SetVariableToString */

/**
 * @}
 */

/**
 * Static checks that ecma types fit size requirements.
 * 
 * Warning:
 *         must not be called
 */
static void __unused
ctx_EcmaTypesSizeCheckers( void)
{
    JERRY_STATIC_ASSERT( sizeof (ecma_Value_t) <= sizeof (uint16_t) );
    JERRY_STATIC_ASSERT( sizeof (ecma_Property_t) <= sizeof (uint64_t) );
    JERRY_STATIC_ASSERT( sizeof (ecma_Object_t) <= sizeof (uint64_t) );
    JERRY_STATIC_ASSERT( sizeof (ecma_ArrayHeader_t) <= sizeof (uint32_t) );
    JERRY_STATIC_ASSERT( sizeof (ecma_ArrayFirstChunk_t) == ECMA_ARRAY_CHUNK_SIZE_IN_BYTES );
    JERRY_STATIC_ASSERT( sizeof (ecma_ArrayNonFirstChunk_t) == ECMA_ARRAY_CHUNK_SIZE_IN_BYTES );

    JERRY_UNREACHABLE();
} /* ctx_EcmaTypesSizeCheckers */

/**
 * @}
 */