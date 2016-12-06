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

INPUT_PATH="$1"
VERSION=$(git log -n 1 --pretty=format:"%H")
SYSTEM=$(uname -a)
TMP_DIR="afl-testing"
USAGE="Usage:\n\t./tools/sort-fails.sh PATH_TO_DIR_WITH_FAILS"

if [ "$#" -ne 1 ]
then
  echo -e "${USAGE}";
  echo "Argument number mismatch...";
  exit 1;
fi

rm "$TMP_DIR" -r; mkdir "$TMP_DIR";

make debug.linux VALGRIND=ON -j && echo "Build OK";

for file in "$INPUT_PATH"/*;
do
  key1=$(./build/bin/debug.linux/jerry --abort-on-fail $file 2>&1);
  key2=$(valgrind --default-suppressions=yes ./build/bin/debug.linux/jerry --abort-on-fail $file 2>&1 \
        | sed -e 's/^==*[0-9][0-9]*==*//g' \
        | sed -e 's/^ Command: .*//g');

  hash1=$(echo -n $key1 | md5sum);
  hash2=$(echo -n $key2 | md5sum);

  dir="$TMP_DIR/$hash1";
  head="$TMP_DIR/$hash1.err.md";
  body="$dir/$hash2.err.md";

  if [ ! -s "$dir" ]; then
    mkdir -p "$dir"; touch "$head";
    echo "###### Version: $VERSION"     >> "$head";
    echo "###### System:"               >> "$head";
    echo '```'                          >> "$head";
    echo "$SYSTEM"                      >> "$head";
    echo '```'                          >> "$head";
    echo "###### Output:"               >> "$head";
    echo '```'                          >> "$head";
    echo "$key1"                        >> "$head";
    echo '```'                          >> "$head";
  fi

  if [ ! -s "$body" ]; then
    touch "$body";
    echo '###### Unique backtrace:```'  >> "$body";
    echo '```'                          >> "$body";
    echo "$key2"                        >> "$body"
    echo '```'                          >> "$body";
  fi

  echo "###### Test case($file):"       >> "$body";
  echo '```js'                          >> "$body";
  cat "$file"                           >> "$body";
  echo '```'                            >> "$body";

done

cd "$TMP_DIR";
for dir in */ ; do
  for file in "$dir"/*;
  do
    cat "$file" >> "${dir%/*}.err.md"
  done
  rm $dir -r;
done
