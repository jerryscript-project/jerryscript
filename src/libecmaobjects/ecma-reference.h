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

#ifndef ECMA_REFERENCE_H
#define ECMA_REFERENCE_H

/** \addtogroup ecma ---TODO---
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
    unsigned int m_IsPropertyReference : 1;

    /**
     * Flag indicating that this reference is strict (see also: ECMA-262 v5, 8.7).
     */
    unsigned int m_StrictReference : 1;

    /**
     * Name of variable (Null-terminated string).
     */
    ecma_Char_t* m_Name;

    /**
     * Name of object's property (Null-terminated string).
     */
    ecma_Char_t* m_PropertyName;
} ecma_SyntacticReference_t;

/**
 * @}
 * @}
 */

#endif /* !ECMA_REFERENCE_H */
