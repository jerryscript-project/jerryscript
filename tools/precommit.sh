#!/bin/bash

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

MAKE="$1"
shift

OUT_DIR="$1"
shift

TARGETS="$1"
shift

commit_hash=`git show -s --format=%H HEAD`
author_name=`git show -s --format=%an HEAD`
author_email=`git show -s --format=%ae HEAD`
required_signed_off_by_line="JerryScript-DCO-1.0-Signed-off-by: $author_name $author_email"
actual_signed_off_by_line=`git show -s --format=%B HEAD | sed '/^$/d' | tail -n 1`

if [ "$actual_signed_off_by_line" != "$required_signed_off_by_line" ]
then
 echo -e "\e[1;33m Signed-off-by message is incorrect. The following line should be at the end of the $commit_hash commit's message: '$required_signed_off_by_line'. \e[0m\n"

 exit 1
fi

VERA_DIRECTORIES_EXCLUDE_LIST="-path ./third-party -o -path tests -o -path ./targets"
VERA_CONFIGURATION_PATH="./tools/vera++"

SOURCES_AND_HEADERS_LIST=`find . -type d \( $VERA_DIRECTORIES_EXCLUDE_LIST \) -prune -or -name "*.c" -or -name "*.cpp" -or -name "*.h"`
./tools/vera++/vera.sh -r $VERA_CONFIGURATION_PATH -p jerry $SOURCES_AND_HEADERS_LIST -e --no-duplicate
STATUS_CODE=$?

if [ $STATUS_CODE -ne 0 ]
then
 echo -e "\e[1;33m vera++ static checks failed. See output above for details. \e[0m\n"

 exit $STATUS_CODE
fi

echo -e "\nBuilding...\n\n"
$MAKE STATIC_CHECK=ON build || exit 1
echo -e "\n================ Build completed successfully. Running precommit tests ================\n"
echo -e "All targets were built successfully. Starting unit tests' run.\n"
$MAKE unittests_run || exit 1
echo -e "\nUnit tests completed successfully. Starting full testing.\n"

./tools/precommit-full-testing.sh "${OUT_DIR}" "${TARGETS}" || exit 1
