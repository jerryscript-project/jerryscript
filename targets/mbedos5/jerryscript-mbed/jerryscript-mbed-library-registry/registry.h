/* Copyright (c) 2016 ARM Limited
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
#ifndef _JERRYSCRIPT_MBED_LIBRARY_REGISTRY_REGISTRY_H
#define _JERRYSCRIPT_MBED_LIBRARY_REGISTRY_REGISTRY_H

#include <vector>
#include "stdint.h"

#define JERRY_USE_MBED_LIBRARY(NAME) \
    mbed::js::LibraryRegistry::getInstance().add(jsmbed_wrap_registry_entry__ ## NAME)

namespace mbed {
namespace js {

typedef void (*library_registration_function_t)(void);

class LibraryRegistry {
 private:
    static LibraryRegistry instance;

 public:
    static LibraryRegistry& getInstance() {
        return instance;
    }

    void add(library_registration_function_t lib_func) {
        funcs.push_back(lib_func);
    }

    void register_all() {
        for (std::size_t i = 0; i < funcs.size(); i++) {
            funcs[i]();
        }
    }

 private:
    LibraryRegistry() {}

    std::vector<library_registration_function_t> funcs;
};

}  // namespace js
}  // namespace mbed

#endif  // _JERRYSCRIPT_MBED_LIBRARY_REGISTRY_REGISTRY_H
