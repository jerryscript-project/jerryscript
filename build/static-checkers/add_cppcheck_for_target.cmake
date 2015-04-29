# Copyright 2015 Samsung Electronics Co., Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Cppcheck launcher
 set(CMAKE_CPPCHECK ${CMAKE_SOURCE_DIR}/tools/cppcheck/cppcheck.sh)

# Definition of cppcheck targets
 add_custom_target(cppcheck)

 function(add_cppcheck_target TARGET_NAME)
  # Get target's parameters
   get_target_property(TARGET_DEFINES  ${TARGET_NAME} COMPILE_DEFINITIONS)
   get_target_property(TARGET_INCLUDES ${TARGET_NAME} INCLUDE_DIRECTORIES)
   get_target_property(TARGET_SOURCES  ${TARGET_NAME} SOURCES)
   get_target_property(TARGET_LIBRARIES ${TARGET_NAME} LINK_LIBRARIES)

  # Build cppcheck's argument strings
   set(CPPCHECK_DEFINES_LIST  )
   set(CPPCHECK_INCLUDES_LIST )
   set(CPPCHECK_SOURCES_LIST  )

   foreach(DEFINE ${TARGET_DEFINES})
    set(CPPCHECK_DEFINES_LIST ${CPPCHECK_DEFINES_LIST} -D${DEFINE})
   endforeach()

   foreach(INCLUDE ${TARGET_INCLUDES})
    set(CPPCHECK_INCLUDES_LIST ${CPPCHECK_INCLUDES_LIST} -I${INCLUDE})
   endforeach()

   set(ADD_CPPCHECK_COMMAND FALSE)

   foreach(SOURCE ${TARGET_SOURCES})
    # Add to list if it is C or C++ source
    get_filename_component(SOURCE_EXTENSION ${SOURCE} EXT)
    if("${SOURCE_EXTENSION}" STREQUAL ".c" OR "${SOURCE_EXTENSION}" STREQUAL ".cpp")
     set(CPPCHECK_SOURCES_LIST ${CPPCHECK_SOURCES_LIST} ${SOURCE})

     set(ADD_CPPCHECK_COMMAND true)
    endif()
   endforeach()

  if(ADD_CPPCHECK_COMMAND)
   add_custom_target(cppcheck.${TARGET_NAME}
                     COMMAND ${CMAKE_CPPCHECK} -j8 --error-exitcode=1 --language=c++ --std=c++11
                                               --enable=warning,style,performance,portability,information
                                               ${CPPCHECK_DEFINES_LIST} ${CPPCHECK_SOURCES_LIST} ${CPPCHECK_INCLUDES_LIST}
                     WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  else()
   add_custom_target(cppcheck.${TARGET_NAME})
  endif()

  if(NOT "${TARGET_LIBRARIES}" STREQUAL "TARGET_LIBRARIES-NOTFOUND")
   foreach(LIBRARY ${TARGET_LIBRARIES})
    string(REGEX MATCH "^${PREFIX_IMPORTED_LIB}.*|.*${SUFFIX_THIRD_PARTY_LIB}$" MATCHED ${LIBRARY})
    if("${MATCHED}" STREQUAL "") # exclude imported and third-party modules
     if(NOT TARGET cppcheck.${LIBRARY})
      add_cppcheck_target(${LIBRARY})

      add_dependencies(cppcheck.${TARGET_NAME} cppcheck.${LIBRARY})
     endif()
    endif()
   endforeach()
  endif()

  add_dependencies(cppcheck cppcheck.${TARGET_NAME})
 endfunction()
