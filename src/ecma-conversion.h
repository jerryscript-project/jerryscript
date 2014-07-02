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
 * \addtogroup ecmaconversion ECMA conversion
 * @{
 */

#ifndef JERRY_ECMA_CONVERSION_H
#define JERRY_ECMA_CONVERSION_H

#include "ecma-defs.h"
#include "ecma-helpers.h"

extern ecma_Object_t* ecma_ToObject( ecma_Value_t value);

/*
 * Stubs
 */

/**
 * Convert value to ecma-object.
 * 
 * See also:
 *          ECMA-262 5.1, 9.9.
 * 
 * @return pointer to ecma-object descriptor
 */
ecma_Object_t*
ecma_ToObject(ecma_Value_t value) /**< ecma-value */
{
    if ( value.m_ValueType == ECMA_TYPE_OBJECT )
    {
        return ecma_DecompressPointer( value.m_Value);
    }
    
    JERRY_UNIMPLEMENTED();
}

#endif /* !JERRY_ECMA_CONVERSION_H */

/**
 * @}
 * @}
 */
