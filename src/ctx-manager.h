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

#ifndef JERRY_CTX_MANAGER_H
#define JERRY_CTX_MANAGER_H

#include "ctx-reference.h"
#include "globals.h"
#include "ecma-defs.h"

/** \addtogroup ctxman Context manager
 * @{
 *  \addtogroup interface Context manager's interface
 * @{
 */

/**
 * Initialize context manager and global execution context.
 */
extern void ctx_Init(void);

/**
 * Create new variables' context using global object
 * for ThisBinding and lexical environments.
 */
extern void ctx_NewContextFromGlobalObject(void);

/**
 * Create new variables' context inheriting lexical environment from specified
 * function's [[Scope]], and setting ThisBinding from pThisVar parameter
 * (see also ECMA-262 5.1, 10.4.3).
 */
extern void ctx_NewContextFromFunctionScope(ctx_SyntacticReference_t *pThisVar, ctx_SyntacticReference_t *pFunctionVar);

/**
 * Create new lexical environment using specified object as binding object,
 * setting provideThis to specified value.
 * The lexical environment is inherited from current context's lexical environment.
 */
extern void ctx_NewLexicalEnvironmentFromObject(ctx_SyntacticReference_t *pObjectVar, bool provideThis);

/**
 * Exit from levelsToExit lexical environments (i.e. choose lexical environment
 * that is levelsToExit outward current lexical environment as new current context's
 * lexical environment).
 */
extern void ctx_ExitLexicalEnvironments(uint32_t levelsToExit);

/**
 * Exit from levelsToExit variables' contexts (i.e. choose context
 * that is levelsToExit from current context as new current context).
 */
extern void ctx_ExitContexts(uint32_t levelsToExit);

/**
 * Create new variable with undefined value.
 */
extern void ctx_NewVariable(ctx_SyntacticReference_t *pVar);

/**
 * Delete specified variable.
 */
extern void ctx_DeleteVariable(ctx_SyntacticReference_t *pVar);

/**
 * Check if specified variable exists
 * 
 * @return true, if exists;
 *         false - otherwise.
 */
extern bool ctx_DoesVariableExist(ctx_SyntacticReference_t *pVar);

/**
 * Copy variable's/property's/array's element's value.
 */
extern void ctx_CopyVariable(ctx_SyntacticReference_t *pVarFrom, ctx_SyntacticReference_t *pVarTo);

/**
 * Get type of specified of variable/property/array's element.
 */
extern ecma_Type_t ctx_GetVariableType(ctx_SyntacticReference_t *pVar);

/**
 * Get specified variable's/property's/array's element's value.
 * 
 * @return number of bytes, actually copied to the buffer, if variable value was copied successfully;
 *         negative number, which is calculated as negation of buffer size, that is required
 *         to hold the variable's value (in case size of buffer is insuficcient).
 */
extern ssize_t ctx_GetVariableValue(ctx_SyntacticReference_t *pVar, uint8_t *pBuffer, size_t bufferSize);

/**
 * Set variable's/property's/array's element's value to one of simple values.
 */
extern void ctx_SetVariableToSimpleValue(ctx_SyntacticReference_t *pVar, ecma_SimpleValue_t value);

/**
 * Set variable's/property's/array's element's value to a Number.
 */
extern void ctx_SetVariableToNumber(ctx_SyntacticReference_t *pVar, ecma_Number_t value);

/**
 * Set variable's/property's/array's element's value to a String.
 */
extern void ctx_SetVariableToString(ctx_SyntacticReference_t *pVar, ecma_Char_t *value, ecma_Length_t length);

#endif /* !JERRY_CTX_MANAGER_H */

/**
 * @}
 * @}
 */