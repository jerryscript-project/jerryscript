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

pylint --version &>/dev/null
if [ $? -ne 0 ]
then
    echo -e "${TERM_RED}Can't run check-pylint because pylint isn't installed.${TERM_NORMAL}\n"
    exit 1
fi

find ./tools ./jerry-debugger -name "*.py" \
    | xargs pylint --rcfile=tools/pylint/pylintrc
