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

#include "ecma-compressed-pointers.h"

#ifndef ECMA_MANAGED_POINTER_H
#define ECMA_MANAGED_POINTER_H

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmamanagedpointers ECMA managed on-stack pointers
 * @{
 */

/**
 * Untyped (generic) managed pointer
 */
class ecma_generic_ptr_t
{
  public:
    /* Constructors */
    __attribute_always_inline__
    ecma_generic_ptr_t () : _ptr (NULL) {}

    ecma_generic_ptr_t (const ecma_generic_ptr_t&) = delete;
    ecma_generic_ptr_t (ecma_generic_ptr_t&) = delete;
    ecma_generic_ptr_t (ecma_generic_ptr_t&&) = delete;

    /* Getter */
    template<typename T>
    operator T () const
    {
      return static_cast<T> (_ptr);
    }

    /* Assignment operators */
    __attribute_always_inline__
    ecma_generic_ptr_t& operator = (void* ptr) /**< new pointer value */
    {
      _ptr = ptr;

      return *this;
    }

    __attribute_always_inline__
    ecma_generic_ptr_t& operator = (const ecma_generic_ptr_t& ptr) /**< managed pointer
                                                                    *   to take value from */
    {
      _ptr = ptr._ptr;

      return *this;
    }

    ecma_generic_ptr_t& operator = (ecma_generic_ptr_t &) = delete;
    ecma_generic_ptr_t& operator = (ecma_generic_ptr_t &&) = delete;

    /* Packing to compressed pointer */
    __attribute_always_inline__
    void pack_to (uintptr_t& compressed_pointer) const /**< reference to compressed pointer */
    {
      ECMA_SET_NON_NULL_POINTER (compressed_pointer, _ptr);
    }

    /* Unpacking from compressed pointer */
    __attribute_always_inline__
    void unpack_from (uintptr_t compressed_pointer) /**< compressed pointer */
    {
      *this = ECMA_GET_NON_NULL_POINTER (void, compressed_pointer);
    }

  protected: /* accessible to ecma_pointer_t */
    void *_ptr; /* pointer storage */
};

/**
 * Typed interface to ecma_generic_ptr_t
 */
template <class T>
class ecma_pointer_t : protected ecma_generic_ptr_t
{
  public:
    /* Member access */
    __attribute_always_inline__
    T* operator -> () const
    {
      return _ptr;
    }

    /* Dereference */
    __attribute_always_inline__
    T operator * () const
    {
      return *_ptr;
    }

    /* Assignment operators */
    __attribute_always_inline__
    ecma_generic_ptr_t& operator = (T* ptr) /**< new pointer value */
    {
      _ptr = ptr;

      return *this;
    }

    __attribute_always_inline__
    ecma_pointer_t<T>& operator = (const T& value) /**< value to assign to variable
                                                    *   under the pointer */
    {
      *_ptr = value;

      return *this;
    }
};

/**
 * @}
 * @}
 */

#endif /* !ECMA_MANAGED_POINTER_H */
