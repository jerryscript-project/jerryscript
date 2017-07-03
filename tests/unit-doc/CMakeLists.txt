# Copyright JS Foundation and other contributors, http://js.foundation
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

cmake_minimum_required (VERSION 2.8.12)
project (unit-doc C)

set(GEN_DOCTEST "${CMAKE_SOURCE_DIR}/tools/gen-doctest.py")
file(GLOB DOC_FILES "${CMAKE_SOURCE_DIR}/docs/*.md")

set(COMPILE_FLAGS_DOCTEST "-Wno-unused-parameter -Wno-unused-function -Wno-unused-variable")

# Dry run of the doctest generator: analyze MarkDown files and get the list of
# file names that will be generated. This allows the definition of proper
# dependencies between the MarkDown files and the generated sources.
execute_process(
  COMMAND ${GEN_DOCTEST} --dry -d ${CMAKE_CURRENT_SOURCE_DIR} ${DOC_FILES}
  OUTPUT_VARIABLE DOCTEST_OUTPUT
  RESULT_VARIABLE GEN_DOCTEST_RESULT
)
if(NOT GEN_DOCTEST_RESULT EQUAL 0)
  message(FATAL_ERROR "failed to get doctest file list")
endif()

# Process the output of the doctest generator: collect sources that must be
# compiled, compiled+linked, or compiled+linked+executed into separate lists.
set(DOCTEST_COMPILE "")
set(DOCTEST_LINK "")
set(DOCTEST_RUN "")

string(REPLACE "\n" ";" DOCTEST_LIST "${DOCTEST_OUTPUT}")
foreach(DOCTEST_ELEMENT IN LISTS DOCTEST_LIST)
  if(NOT ("${DOCTEST_ELEMENT}" STREQUAL ""))
    separate_arguments(DOCTEST_ELEMENT)
    list(GET DOCTEST_ELEMENT 0 DOCTEST_ACTION)
    list(GET DOCTEST_ELEMENT 1 DOCTEST_NAME)
    string(TOUPPER ${DOCTEST_ACTION} DOCTEST_ACTION)

    set(DOCTEST_${DOCTEST_ACTION} ${DOCTEST_${DOCTEST_ACTION}} ${DOCTEST_NAME})
  endif()
endforeach()

# Add custom command to run doctest generator if any of the MarkDown sources
# changes.
add_custom_command(
  COMMAND ${GEN_DOCTEST} -d ${CMAKE_CURRENT_SOURCE_DIR} ${DOC_FILES}
  DEPENDS ${GEN_DOCTEST} ${DOC_FILES}
  OUTPUT ${DOCTEST_COMPILE} ${DOCTEST_LINK} ${DOCTEST_RUN}
  COMMENT "Generating doctests"
)

# Process compile-only doctests: add them to a dummy library
# (named libcompile-doc-tests.a) to trigger compilation.
if(NOT ("${DOCTEST_COMPILE}" STREQUAL ""))
  add_library(compile-doc-tests STATIC ${DOCTEST_COMPILE})
  target_link_libraries(compile-doc-tests jerry-ext jerry-core jerry-port-default-minimal)
  set_property(TARGET compile-doc-tests APPEND_STRING PROPERTY COMPILE_FLAGS "${COMPILE_FLAGS_DOCTEST}")
endif()

macro(doctest_add_executables NAME_PREFIX)
  foreach(DOCTEST_NAME ${ARGN})
    get_filename_component(TARGET_NAME ${DOCTEST_NAME} NAME)
    string(REGEX REPLACE "\\.[^.]*$" "" TARGET_NAME ${TARGET_NAME})
    set(TARGET_NAME ${NAME_PREFIX}-${TARGET_NAME})

    add_executable(${TARGET_NAME} ${DOCTEST_NAME})
    set_property(TARGET ${TARGET_NAME} APPEND_STRING PROPERTY COMPILE_FLAGS "${COMPILE_FLAGS_DOCTEST}")
    set_property(TARGET ${TARGET_NAME} PROPERTY LINK_FLAGS "${LINKER_FLAGS_COMMON}")
    target_link_libraries(${TARGET_NAME} jerry-ext jerry-core jerry-port-default-minimal)
  endforeach()
endmacro()

# Process link-only doctests: create an executable from each (named link-doc-*)
doctest_add_executables("link-doc" ${DOCTEST_LINK})

# Process "full-fledged" doctests: create an executable from each (named
# unit-doc-*). Their name prefix (unit-) ensures that the unit test runner
# script will treat them like any other unit test, i.e., executed them.
doctest_add_executables("unit-doc" ${DOCTEST_RUN})
