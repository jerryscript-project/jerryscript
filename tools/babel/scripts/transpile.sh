#!/usr/bin/env bash

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

if [[ "$#" -gt 2 || "$#" -lt 1 ]]; then
  echo "Usage: $0 input [output]"
  echo "* input: name of input directory/file"
  echo "* output: name of output directory/file (same as input if not given)"
  exit 1
fi

if [[ ! -d $1 && ! -f $1 ]]; then
  echo "Error: $1 is not a file or directory"
  exit 1
fi

FLAG='--out-file'
if [[ -d $1 ]]; then
  FLAG='--out-dir'
fi

if [[ "$#" -eq 1 ]]; then
  ./node_modules/.bin/babel $1 $FLAG $1 --verbose
else
  ./node_modules/.bin/babel $1 $FLAG $2 --verbose
fi
