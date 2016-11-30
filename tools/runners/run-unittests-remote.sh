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

TARGET_IP="$1"
TARGET_USER="$2"
TARGET_PASS="$3"

if [ $# -lt 3 ]
then
  echo "This script runs unittests on the remote board."
  echo ""
  echo "Usage:"
  echo "  1st parameter: ip address of target board: {110.110.110.110}"
  echo "  2nd parameter: ssh login to target board: {login}"
  echo "  3rd parameter: ssh password to target board: {password}"
  echo ""
  echo "Example:"
  echo "  ./tools/runners/run-unittests-remote.sh 110.110.110.110 login password"
  exit 1
fi

BASE_DIR=$(dirname "$(readlink -f "$0")" )

OUT_DIR="${BASE_DIR}"/../.././build/bin

rm -rf "${OUT_DIR}"/unittests/check

mkdir -p "${OUT_DIR}"/unittests/check

export SSHPASS="${TARGET_PASS}"
REMOTE_TMP_DIR=$(sshpass -e ssh "${TARGET_USER}"@"${TARGET_IP}" 'mktemp -d')

sshpass -e scp "${BASE_DIR}"/../../tools/runners/run-unittests.sh "${TARGET_USER}"@"${TARGET_IP}":"${REMOTE_TMP_DIR}"
sshpass -e scp -r "${OUT_DIR}"/unittests/* "${TARGET_USER}"@"${TARGET_IP}":"${REMOTE_TMP_DIR}"

sshpass -e ssh "${TARGET_USER}"@"${TARGET_IP}" "mkdir -p \"${REMOTE_TMP_DIR}\"/check; \
                                                \"${REMOTE_TMP_DIR}\"/run-unittests.sh \"${REMOTE_TMP_DIR}\"; \
                                                echo \$? > \"${REMOTE_TMP_DIR}\"/check/IS_REMOTE_TEST_OK"

sshpass -e scp -r "${TARGET_USER}"@$"{TARGET_IP}":"${REMOTE_TMP_DIR}"/check "${OUT_DIR}"/unittests

sshpass -e ssh "${TARGET_USER}"@"${TARGET_IP}" "rm -rf \"${REMOTE_TMP_DIR}\""

STATUS=$(cat "${OUT_DIR}"/unittests/check/IS_REMOTE_TEST_OK)

if [ "${STATUS}" == 0 ] ; then
    echo "Unit tests run passed."
    exit 0
else
    echo "Unit tests run failed. See ${OUT_DIR}/unittests/unit_tests_run.log for details."
    exit 1
fi
