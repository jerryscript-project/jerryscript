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

TERM_NORMAL='\033[0m'
TERM_RED='\033[1;31m'
IGNORE_PATH='./tools/eslint/.eslintignore'

eslint --version &>/dev/null
if [ $? -ne 0 ]; then
    echo -e "${TERM_RED}Can't run check-eslint because eslint isn't installed.${TERM_NORMAL}\n"
    echo -e "Install command:"
    echo -e "\$ npm install -g eslint\n"
    exit 1
fi

FIX=""
if [ "$1" = "--fix" ]; then
    FIX=$1
fi

# Run all tests as script
eslint -c ./tools/eslint/.eslintrc.js --ignore-path $IGNORE_PATH $FIX ./tests/jerry

if [ $? -ne 0 ]; then
    exit 1
fi

# Run module tests
eslint -c ./tools/eslint/.eslintrc.js --parser-options=ecmaVersion:11,sourceType:module $FIX tests/jerry/es.next/module*
