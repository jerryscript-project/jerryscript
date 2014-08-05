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

GIT_STATUS_NOT_CLEAN_MSG="Git status of current directory is not clean"
GIT_STATUS_CONSIDER_CLEAN_MSG="Consider removing all untracked files, locally commiting all changes and running $0 again"

trap ctrl_c INT

function ctrl_c() {
    git checkout master >&/dev/null

    exit 1
}

clear

if [ "`git status --porcelain 2>&1 | wc -l`" != "0" ]
then
  echo -e "\n  \e[1;90m$GIT_STATUS_NOT_CLEAN_MSG:\n"
  git status
  echo -e "\n\n  $GIT_STATUS_CONSIDER_CLEAN_MSG.\e[0m\n"
fi

ok_to_push=1

current_branch=`git branch | grep "^* " | cut -d ' ' -f 2`

if [ "$current_branch" != "master" ]
then
  echo "Current branch is '$current_branch', not 'master'."

  exit 1
fi

commits_to_push=`git log origin/master..master | grep "^commit [0-9a-f]*$" | awk 'BEGIN { s = "";} {s = $2" "s;} END {print s;}'`

echo
echo "===== Starting pre-push commit testing series ====="
echo
echo "Commits list: $commits_to_push"
echo

for commit_hash in $commits_to_push
do
  echo " > Testing $commit_hash"
  echo

  git checkout $commit_hash >&/dev/null
  status_code=$?
  if [ $status_code -ne 0 ]
  then
    echo "git checkout $commit_hash failed"

    exit 1
  fi

  precommit_testing_output=`make -j precommit 2>&1`
  status_code=$?
  if [ $status_code -ne 0 ]
  then
    echo "Pre-commit testing for '$commit_hash' failed"
    echo
    echo $precommit_testing_output
    echo

    ok_to_push=0
    break
  fi
done

git checkout master >&/dev/null

echo
echo "Pre-commit testing passed successfully"
echo

if [ $ok_to_push -eq 1 ]
then
  if [ "`git status --porcelain 2>&1 | grep -v -e '^?? out/$$' | wc -l`" == "0" ]
  then
    echo "git push"
    echo

    git push
    exit 0
  else
    echo -e "\e[1;33m $GIT_STATUS_NOT_CLEAN_MSG. $GIT_STATUS_CONSIDER_CLEAN_MSG.\e[0m\n"

    exit 1
  fi
else
  echo "\e[1;33mPre-commit testing not passed. Cancelling push.\e[0m"

  exit 1
fi
