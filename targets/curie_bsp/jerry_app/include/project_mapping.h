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

/**
 * Allow project to override this partition scheme
 * The following variables are allowed to be defined:
 *
 * QUARK_START_PAGE the first page where the QUARK code is located
 * QUARK_NB_PAGE the number of pages reserved for the QUARK. The ARC gets the
 *               remaining pages (out of 148).
 */
#ifndef PROJECT_MAPPING_H
#define PROJECT_MAPPING_H

#define QUARK_NB_PAGE 125
#include "machine/soc/intel/quark_se/quark_se_mapping.h"

#endif /* !PROJECT_MAPPING_H */
