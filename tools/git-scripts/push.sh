#!/bin/bash

# Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

GIT_STATUS_NOT_CLEAN_MSG="Git status of current directory is not clean"
GIT_STATUS_CONSIDER_CLEAN_MSG="Consider removing all untracked files, locally commiting all changes and running $0 again"

clear

gitignore_files_list=`find . -name .gitignore`

if [ "$gitignore_files_list" != "./.gitignore" ]
then
  echo -e "\n\e[1;33mInvalid .gitignore configuration\e[0m\n"
  echo -e -n ".gitignore files list:\t"
  echo $gitignore_files_list
  echo

  exit 1
fi

if [ "`git status --porcelain 2>&1 | wc -l`" != "0" ]
then
  echo -e "\n  \e[1;90m$GIT_STATUS_NOT_CLEAN_MSG:\n"
  git status
  echo -e "\n\n  $GIT_STATUS_CONSIDER_CLEAN_MSG.\e[0m\n"
fi

ok_to_push=1

current_branch=`git branch | grep "^* " | cut -d ' ' -f 2`
git branch -r | grep "^ *origin/$current_branch$" 2>&1 > /dev/null
have_remote=$?

if [ $have_remote -eq 0 ]
then
  base_ref="origin/$current_branch"

  echo "Pulling..."

  make pull
  status_code=$?

  if [ $status_code -ne 0 ]
  then
    echo "Pull failed"
    exit 1
  fi
else
  base_ref=`git merge-base master $current_branch`
  status_code=$?

  if [ $status_code -ne 0 ]
  then
    echo "Cannot determine merge-base for '$current_branch' and 'master' branches."
    exit 1
  fi
fi

commits_to_push=`git log $base_ref..$current_branch | grep "^commit [0-9a-f]*$" | awk 'BEGIN { s = ""; } { s = $2" "s; } END { print s; }'`

echo $commits_to_push | grep "[^ ]" >&/dev/null
status_code=$?
if [ $status_code -ne 0 ]
then
  echo "Nothing to push"
  exit 0
fi

trap ctrl_c INT

function ctrl_c() {
    git checkout $current_branch >&/dev/null

    exit 1
}

echo
echo "===== Starting pre-push commit testing series ====="
echo
echo "Commits list: $commits_to_push"
echo

for commit_hash in $commits_to_push
do
  git checkout $commit_hash >&/dev/null
  status_code=$?

  if [ $status_code -ne 0 ]
  then
    echo "git checkout $commit_hash failed"

    exit 1
  fi

  echo " > Testing $commit_hash"
  echo -n " > "
  git log  --pretty=format:"%H %s" | grep $commit_hash | grep -o " .*"
  echo

  make -s -j precommit 2>&1
  status_code=$?
  if [ $status_code -ne 0 ]
  then
    echo "Pre-commit quality testing for '$commit_hash' failed"
    echo

    ok_to_push=0
    break
  fi

  echo "Pre-commit quality testing for '$commit_hash' passed successfully"
done

git checkout $current_branch >&/dev/null

echo
echo "Pre-commit testing passed successfully"
echo

if [ $ok_to_push -eq 1 ]
then
  if [ "`git status --porcelain 2>&1 | wc -l`" == "0" ]
  then
    echo "Pushing..."
    echo

    git push -u origin $current_branch
    status_code=$?

    if [ $status_code -eq 0 ]
    then
      echo -e "\n\e[0;32m     Pushed successfully\e[0m\n"
    else
      echo -e "\n\e[1;33m     Push failed\e[0m"
    fi

    exit $status_code
  else
    echo -e "\e[1;33m $GIT_STATUS_NOT_CLEAN_MSG. $GIT_STATUS_CONSIDER_CLEAN_MSG.\e[0m\n"

    exit 1
  fi
else
  echo -e "\e[1;33mPre-commit testing not passed. Cancelling push.\e[0m"

  exit 1
fi
