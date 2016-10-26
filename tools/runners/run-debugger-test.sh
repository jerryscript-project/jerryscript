#!/bin/bash

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

JERRY=$1
DEBUGGER_CLIENT=$2
TEST_CASE=$3

START_DEBUG_SERVER="${JERRY} ${TEST_CASE}.js --start-debug-server &"

echo "$START_DEBUG_SERVER"
eval "$START_DEBUG_SERVER"
sleep 1s

RESULT=$((cat "${TEST_CASE}.cmd" | ${DEBUGGER_CLIENT}) 2>&1)
DIFF=$(diff -u0 ${TEST_CASE}.expected <(echo "$RESULT"))

if [ -n "$DIFF" ]
then
	echo "$DIFF"
	echo "${TEST_CASE} failed"
	exit 1
fi

echo "${TEST_CASE} passed"
exit 0
