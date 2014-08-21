# Copyright 2014 Samsung Electronics Co., Ltd.
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

#!/bin/bash

trap ctrl_c INT

function ctrl_c() {
    git checkout master >&/dev/null

    exit 1
}

git pull --rebase
status_code=$?

if [ $status_code -ne 0 ]
then
  echo "Pulling master failed"

  exit 1
fi

for notes_ref in perf mem test_build_env
do
  git checkout refs/notes/$notes_ref
  git pull --rebase origin refs/notes/$notes_ref
done

git checkout master >&/dev/null
