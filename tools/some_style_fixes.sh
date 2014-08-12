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

if [[ "$1" != "-y" || ! -d "$2" ]]
then
  echo "Transformation validity was not checked for all cases."
  echo "Please, check code after fixing it with this script".
  echo "To run script pass -y in first argument and directory path in the second."

  exit 1
fi

dir=$2
# Remove trailing spaces
find $dir/*.[chS] -exec sed -i 's/[[:space:]]\{1,\}$//g' {} \;

# Remove tabs
find $dir/*.[chS] -exec sed -i 's/\t/  /g' {} \;

# Add space between function name and opening parentheses
find $dir/*.[chS] -exec sed -i 's/\([a-z0-9_-]\)\((\)/\1 \2/g' {} \;

# Remove space after opening parentheses 
find $dir/*.[chS] -exec sed -i 's/([ ]*/(/g' {} \;

# Remove space before closing parentheses 
find $dir/*.[chS] -exec sed -i 's/[ ]*)/)/g' {} \;

# Place else on separate line
find $dir/*.[chS] -exec sed -i 's/\([ ]*\)} else\(.*\)/\1}\n\1else\2/g' {} \;

