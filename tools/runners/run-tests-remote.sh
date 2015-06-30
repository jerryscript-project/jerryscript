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

TARGET="$1" #debug.linux release.linux
TARGET_IP="$2" # ip address of target board
TARGET_USER="$3" # login
TARGET_PASS="$4" # password

if [ $# -lt 4 ]
then
  echo "This script runs ./jerry/* and ./jerry-test-suite/* tests on the remote board."
  echo ""
  echo "Usage:"
  echo "  1st parameter: target to be tested: {debug.linux, release.linux}"
  echo "  2nd parameter: ip address of target board: {110.110.110.110}"
  echo "  3rd parameter: ssh login to target board: {login}"
  echo "  4th parameter: ssh password to target board: {password}"
  echo ""
  echo "Example:"
  echo "  ./tools/runners/run-tests-remote.sh debug.linux 110.110.110.110 login password"
  exit 1
fi

BASE_DIR=$(dirname "$(readlink -f "$0")" )

OUT_DIR="${BASE_DIR}"/../.././build/bin
LOGS_PATH_FULL="${OUT_DIR}"/"${TARGET}"/check

export SSHPASS="${TARGET_PASS}"

rm -rf "${LOGS_PATH_FULL}"

mkdir -p "${LOGS_PATH_FULL}"

REMOTE_TMP_DIR=$(sshpass -e ssh "${TARGET_USER}"@"${TARGET_IP}" 'mktemp -d')
REMOTE_TMP_TAR=$(sshpass -e ssh "${TARGET_USER}"@"${TARGET_IP}" 'mktemp')
LOCAL_TMP_TAR=$(mktemp)

tar -zcf "${LOCAL_TMP_TAR}" \
  "${BASE_DIR}"/../.././build/bin/"${TARGET}" \
  "${BASE_DIR}"/../.././tests/benchmarks \
  "${BASE_DIR}"/../.././tests/jerry \
  "${BASE_DIR}"/../.././tests/jerry-test-suite \
  "${BASE_DIR}"/../.././tools/runners \
  "${BASE_DIR}"/../.././tools/precommit-full-testing.sh > /dev/null 2>&1

sshpass -e scp "${LOCAL_TMP_TAR}" "${TARGET_USER}"@"${TARGET_IP}":"${REMOTE_TMP_TAR}"

sshpass -e ssh "${TARGET_USER}"@"${TARGET_IP}" \
  "tar -zxf \"${REMOTE_TMP_TAR}\" -C \"${REMOTE_TMP_DIR}\"; rm \"${REMOTE_TMP_TAR}\";\
  cd \"${REMOTE_TMP_DIR}\"; \
  ./tools/precommit-full-testing.sh ./build/bin \"$TARGET\" > ./build/bin/\"${TARGET}\"/check/run.log 2>&1; \
  echo \$? > ./build/bin/\"${TARGET}\"/check/IS_TEST_OK"

sshpass -e scp -r "${TARGET_USER}"@"${TARGET_IP}":"${REMOTE_TMP_DIR}"/build/bin/"${TARGET}"/check/* "${LOGS_PATH_FULL}"
sshpass -e ssh "${TARGET_USER}"@"${TARGET_IP}" "rm -rf \"${REMOTE_TMP_DIR}\""

STATUS=$(cat "${LOGS_PATH_FULL}"/IS_TEST_OK)

if [ "${STATUS}" == 0 ] ; then
  echo "${TARGET} testing passed."
  exit 0
else
  echo "${TARGET} testing failed."
  echo "See logs in ${LOGS_PATH_FULL} directory for details."
  exit 1
fi