/* Copyright JS Foundation and other contributors, http://js.foundation
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

#include "ecma-builtin-date.h"
#include "ecma-date-object.h"
#include "ecma-globals.h"


/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmadateobject ECMA Date object related routines
 * @{
 */

ecma_value_t
ecma_date_create (ecma_number_t tv)
{
  return ecma_builtin_date_create (tv);
}

ecma_number_t
ecma_date_now (void)
{
  return ecma_builtin_date_now_helper ();
}

ecma_number_t
ecma_date_parse (ecma_string_t *str_p)
{
  return ecma_builtin_date_parse (str_p);
}

ecma_value_t
ecma_date_get_number_value (ecma_value_t date_value)
{
  // TODO check if date_value is date object
#if JERRY_ESNEXT
  ecma_date_object_t *date_object_p = (ecma_date_object_t *) this_obj_p;
  ecma_number_t *date_value_p = &date_object_p->date_value;
#else
  ecma_extended_object_t *ext_object_p = (ecma_extended_object_t *) this_obj_p;
  ecma_number_t *date_value_p = ECMA_GET_INTERNAL_VALUE_POINTER (ecma_number_t,
                                                                 ext_object_p->u.class_prop.u.date);
#endif

  return ecma_make_number_value (*date_value_p);
}

/**
 * @}
 * @}
 */
