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

OUT_FILE=$1

GIT_BRANCH=$(git symbolic-ref -q HEAD)
GIT_HASH=$(git rev-parse HEAD)
BUILD_DATE=$(date +'%d/%m/%Y')

echo "#ifndef VERSION_H"                                           >  $OUT_FILE
echo "#define VERSION_H"                                           >> $OUT_FILE
echo ""                                                            >> $OUT_FILE
echo "static const char *jerry_build_date = \"$BUILD_DATE\";"      >> $OUT_FILE
echo "static const char *jerry_commit_hash = \"$GIT_HASH\";"       >> $OUT_FILE
echo "static const char *jerry_branch_name = \"$GIT_BRANCH\";"     >> $OUT_FILE
echo ""                                                            >> $OUT_FILE
echo "#endif	/* VERSION_H */"                                   >> $OUT_FILE

