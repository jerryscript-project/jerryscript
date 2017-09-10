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
CLIENT_ARGS=""

if [[ $TEST_CASE == *"client_source"* ]]; then
  START_DEBUG_SERVER="${JERRY} --start-debug-server --debugger-wait-source &"
  if [[ $TEST_CASE == *"client_source_multiple"* ]]; then
    CLIENT_ARGS="--client-source ${TEST_CASE}_2.js ${TEST_CASE}_1.js"
  else
    CLIENT_ARGS="--client-source ${TEST_CASE}.js"
  fi
else
  START_DEBUG_SERVER="${JERRY} ${TEST_CASE}.js --start-debug-server &"
fi

echo "$START_DEBUG_SERVER"
eval "$START_DEBUG_SERVER"
sleep 1s

RESULT_TEMP=`mktemp ${TEST_CASE}.out.XXXXXXXXXX`

(cat "${TEST_CASE}.cmd" | ${DEBUGGER_CLIENT} --non-interactive ${CLIENT_ARGS}) &> ${RESULT_TEMP}
diff -U0 ${TEST_CASE}.expected ${RESULT_TEMP}
STATUS_CODE=$?

rm -f ${RESULT_TEMP}

if [ ${STATUS_CODE} -ne 0 ]
then
  echo "${TEST_CASE} failed"
else
  echo "${TEST_CASE} passed"
fi

exit ${STATUS_CODE}
