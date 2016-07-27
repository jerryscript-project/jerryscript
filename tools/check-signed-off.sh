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

author_name=`git show -s --format=%an $commit_hash`
author_email=`git show -s --format=%ae $commit_hash`
required_signed_off_by_line="JerryScript-DCO-1.0-Signed-off-by: $author_name $author_email"
actual_signed_off_by_line=`git show -s --format=%B $commit_hash | sed '/^$/d' | tr -d '\015' | tail -n 1`

if [ "$actual_signed_off_by_line" != "$required_signed_off_by_line" ]
then
 echo -e "\e[1;33mSigned-off-by message is incorrect. The following line should be at the end of the $commit_hash commit's message: '$required_signed_off_by_line'. \e[0m"

 exit 1
fi

exit 0
