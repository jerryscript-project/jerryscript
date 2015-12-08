/* Copyright 2015 Samsung Electronics Co., Ltd.
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

#ifndef __JERRY_EXTAPI_H__
#define __JERRY_EXTAPI_H__

#define JERRY_STANDALONE_EXIT_CODE_OK   (0)
#define JERRY_STANDALONE_EXIT_CODE_FAIL (1)

#define API_DATA_IS_OBJECT(val_p) \
    ((val_p)->type == JERRY_API_DATA_TYPE_OBJECT)

#define API_DATA_IS_FUNCTION(val_p) \
    (API_DATA_IS_OBJECT(val_p) && \
     jerry_api_is_function((val_p)->v_object))

#define JS_VALUE_TO_NUMBER(val_p) \
    ((val_p)->type == JERRY_API_DATA_TYPE_FLOAT32 ? \
       static_cast<double>((val_p)->v_float32) : \
     (val_p)->type == JERRY_API_DATA_TYPE_FLOAT64 ? \
       static_cast<double>((val_p)->v_float64) : \
     static_cast<double>((val_p)->v_uint32))


#ifdef __cplusplus
extern "C" {
#endif


void js_register_functions (void);


#ifdef __cplusplus
}
#endif

#endif
