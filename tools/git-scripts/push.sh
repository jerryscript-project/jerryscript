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

CPPCHECK_INFO=`cppcheck --version`
VERA_INFO="Vera++ "`vera++ --version`
GCC_INFO=`gcc --version | head -n 1`
BUILD_INFO=`echo -e "$CPPCHECK_INFO\n$VERA_INFO\n$GCC_INFO"`

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

echo "Pulling..."

make pull
status_code=$?

if [ $status_code -ne 0 ]
then
  echo "Pull failed"
  exit 1
fi

ok_to_push=1

current_branch=`git branch | grep "^* " | cut -d ' ' -f 2`

if [ "$current_branch" != "master" ]
then
  echo "Current branch is '$current_branch', not 'master'."

  exit 1
fi

commits_to_push=`git log origin/master..master | grep "^commit [0-9a-f]*$" | awk 'BEGIN { s = ""; } { s = $2" "s; } END { print s; }'`

echo $commits_to_push | grep "[^ ]" >&/dev/null
status_code=$?
if [ $status_code -ne 0 ]
then
  echo "Nothing to push"
  exit 0
fi

trap ctrl_c INT

function ctrl_c() {
    git checkout master >&/dev/null

#    for commit_hash in $commits_to_push
#    do
#      git notes --ref=test_build_env remove $commit_hash
#      git notes --ref=perf remove $commit_hash
#      git notes --ref=mem remove $commit_hash
#    done

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

#  git notes --ref=test_build_env remove $commit_hash >&/dev/null
#  git notes --ref=perf remove $commit_hash >&/dev/null
#  git notes --ref=mem remove $commit_hash >&/dev/null

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
#  echo
#  echo "Starting pre-commit performance measurement for '$commit_hash'"
#  echo

#  BENCH_ENGINE="./build/bin/release.linux/jerry"
#  BENCH_SCRIPT="./tests/benchmarks/jerry/loop_arithmetics_1kk.js"
#  PERF_ITERS="5"
#  PERF_INFO=`echo -e "$BENCH_SCRIPT:\n\t"``./tools/perf.sh $PERF_ITERS $BENCH_ENGINE $BENCH_SCRIPT`" seconds"
#  MEM_INFO=`echo -e "$BENCH_SCRIPT:\n"``./tools/rss-measure.sh $BENCH_ENGINE $BENCH_SCRIPT`

#  echo "Pre-commit performance measurement for '$commit_hash' completed"
#  echo

#  git notes --ref=test_build_env add -m "$BUILD_INFO" $commit_hash
#  git notes --ref=perf add -m "$PERF_INFO" $commit_hash
#  git notes --ref=mem add -m "$MEM_INFO" $commit_hash
done

git checkout master >&/dev/null

echo
echo "Pre-commit testing passed successfully"
echo

if [ $ok_to_push -eq 1 ]
then
  if [ "`git status --porcelain 2>&1 | wc -l`" == "0" ]
  then
    echo "Pushing..."
    echo

    git push origin master # refs/notes/*
    status_code=$?

    if [ $status_code -eq 0 ]
    then
      echo -e "\n\e[0;32m     Pushed successfully\e[0m\n"
    else
      echo -e "\n\e[1;33m     Push failed\e[0m"

#      for commit_hash in $commits_to_push
#      do
#        git notes --ref=test_build_env remove $commit_hash
#        git notes --ref=perf remove $commit_hash
#        git notes --ref=mem remove $commit_hash
#      done
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
