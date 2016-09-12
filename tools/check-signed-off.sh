#!/bin/bash

# Copyright 2015-2016 Samsung Electronics Co., Ltd.
# Copyright 2016 University of Szeged
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

# Usage
function print_usage
{
 echo "Usage: $0 [--help] [--tolerant] [--travis]"
}

function print_help
{
 echo "$0: Check Signed-off-by message of the latest commit"
 echo ""
 print_usage
 echo ""
 echo "Optional arguments:"
 echo "  --help            print this help message"
 echo "  --tolerant        check the existence of the message only but don't"
 echo "                    require the name and email address to match the author"
 echo "                    of the commit"
 echo "  --travis          perform check in tolerant mode if on Travis CI and not"
 echo "                    checking a pull request, perform strict check otherwise"
 echo ""
 echo "The last line of every commit message must follow the form of:"
 echo "'JerryScript-DCO-1.0-Signed-off-by: NAME EMAIL', where NAME and EMAIL must"
 echo "match the name and email address of the author of the commit (unless in"
 echo "tolerant mode)."
}

# Processing command line
TOLERANT="no"
while [ "$#" -gt 0 ]
do
 if [ "$1" == "--help" ]
 then
  print_help
  exit 0
 elif [ "$1" == "--tolerant" ]
 then
  TOLERANT="yes"
  shift
 elif [ "$1" == "--travis" ]
 then
  if [ "$TRAVIS_PULL_REQUEST" == "" ]
  then
   echo -e "\e[1;33mWarning! Travis-tolerant mode requested but not running on Travis CI! \e[0m"
  elif [ "$TRAVIS_PULL_REQUEST" == "false" ]
  then
   TOLERANT="yes"
  else
   TOLERANT="no"
  fi
  shift
 else
  print_usage
  exit 1
 fi
done

# Determining latest commit
parent_hashes=(`git show -s --format=%p HEAD | head -1`)

if [ "${#parent_hashes[@]}" -eq 1 ]
then
 commit_hash=`git show -s --format=%h HEAD | head -1`
elif [ "${#parent_hashes[@]}" -eq 2 ]
then
 commit_hash=${parent_hashes[1]}
else
 echo "$0: cannot handle commit with ${#parent_hashes[@]} parents ${parent_hashes[@]}"
 exit 1
fi

# Checking the last line
actual_signed_off_by_line=`git show -s --format=%B $commit_hash | sed '/^$/d' | tr -d '\015' | tail -n 1`

if [ "$TOLERANT" == "no" ]
then
 author_name=`git show -s --format=%an $commit_hash`
 author_email=`git show -s --format=%ae $commit_hash`
 required_signed_off_by_line="JerryScript-DCO-1.0-Signed-off-by: $author_name $author_email"

 if [ "$actual_signed_off_by_line" != "$required_signed_off_by_line" ]
 then
  echo -e "\e[1;33mSigned-off-by message is incorrect. The following line should be at the end of the $commit_hash commit's message: '$required_signed_off_by_line'. \e[0m"
  exit 1
 fi
else
  echo -e "\e[1;33mWarning! The name and email address of the author of the $commit_hash commit is not checked in tolerant mode! \e[0m"
  if echo "$actual_signed_off_by_line" | grep -q -v '^JerryScript-DCO-1.0-Signed-off-by:'
  then
   echo -e "\e[1;33mSigned-off-by message is incorrect. The following line should be at the end of the $commit_hash commit's message: '$required_signed_off_by_line'. \e[0m"
   exit 1
  fi
fi

exit 0
