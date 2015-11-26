#!/bin/bash

# Copyright 2015 Samsung Electronics Co., Ltd.
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

PREREQUISITES_INSTALLED_LIST_FILE="$1"
shift

if [ "$1" == "clean" ]
then
  CLEAN_MODE=yes
else
  CLEAN_MODE=no
fi

trap clean_on_exit INT

function clean_on_exit() {
  rm -rf $TMP_DIR

  [[ $1 == "OK" ]] || exit 1
  exit 0
}

function fail_msg() {
  echo "$1"

  clean_on_exit "FAIL"
}

function remove_gitignore_files_at() {
  DEST="$1"
  gitignore_files_list=`find "$DEST" -name .gitignore`
  [ $? -eq 0 ] || fail_msg "Failed to search for .gitignore in '$DEST'."

  rm -rf $gitignore_files_list || fail_msg "Failed to remove .gitignore files from '$DEST'."
}

function setup_from_zip() {
  NAME="$1"
  shift

  DEST=$(pwd)/"$1"
  shift

  URL="$1"
  shift

  CHECKSUM="$1"
  shift

  LIST="$*"

  FAIL_MSG="Failed to setup '$NAME' prerequisite"

  if [ "$CLEAN_MODE" == "no" ]
  then
    echo "$CHECKSUM $NAME" >> $TMP_DIR/.prerequisites
    grep -q "^$CHECKSUM $NAME\$" $TMP_DIR/.prerequisites.prev && return 0

    echo "Setting up $NAME prerequisite"
  fi

  if [ -e "$DEST" ]
  then
    chmod -R u+w "$DEST" || fail_msg "$FAIL_MSG. Failed to add write permission to '$DEST' directory contents."
    rm -rf "$DEST" || fail_msg "$FAIL_MSG. Cannot remove '$DEST' directory."
  fi

  if [ "$CLEAN_MODE" == "yes" ]
  then
    return 0
  fi

  wget --no-check-certificate -O "$TMP_DIR/$NAME.zip" "$URL" || fail_msg "$FAIL_MSG. Cannot download '$URL' zip archive."

  echo "$CHECKSUM  $TMP_DIR/$NAME.zip" | $SHA256SUM --check || fail_msg "$FAIL_MSG. Archive's checksum doesn't match."

  unzip "$TMP_DIR/$NAME.zip" -d "$TMP_DIR/$NAME" || fail_msg "$FAIL_MSG. Failed to unpack zip archive."
  mkdir "$DEST" || fail_msg "$FAIL_MSG. Failed to create '$DEST' directory."

  for part in "$LIST"
  do
    mv "$TMP_DIR/$NAME"/$part "$DEST" || fail_msg "$FAIL_MSG. Failed to move '$part' to '$DEST'."
  done

  remove_gitignore_files_at "$DEST"
  chmod -R u-w "$DEST" || fail_msg "$FAIL_MSG. Failed to remove write permission from '$DEST' directory contents."
}

function setup_cppcheck() {
  NAME="$1"
  shift

  DEST=$(pwd)/"$1"
  shift

  URL="$1"
  shift

  CHECKSUM="$1"
  shift

  FAIL_MSG="Failed to setup '$NAME' prerequisite"

  if [ "$CLEAN_MODE" == "no" ]
  then
    echo "$CHECKSUM $NAME" >> $TMP_DIR/.prerequisites
    grep -q "^$CHECKSUM $NAME\$" $TMP_DIR/.prerequisites.prev && return 0

    echo "Setting up $NAME prerequisite"
  fi

  if [ -e "$DEST" ]
  then
    chmod -R u+w "$DEST" || fail_msg "$FAIL_MSG. Failed to add write permission to '$DEST' directory contents."
    rm -rf "$DEST" || fail_msg "$FAIL_MSG. Cannot remove '$DEST' directory."
  fi

  if [ "$CLEAN_MODE" == "yes" ]
  then
    return 0
  fi

  wget --no-check-certificate -O "$TMP_DIR/$NAME.tar.bz2" "$URL" || fail_msg "$FAIL_MSG. Cannot download '$URL' archive."

  echo "$CHECKSUM  $TMP_DIR/$NAME.tar.bz2" | $SHA256SUM --check || fail_msg "$FAIL_MSG. Archive's checksum doesn't match."

  tar xjvf "$TMP_DIR/$NAME.tar.bz2" -C "$TMP_DIR" || fail_msg "$FAIL_MSG. Failed to unpack archive."

  (
    cd "$TMP_DIR/$NAME" || exit 1
    make -j HAVE_RULES=yes CFGDIR="$DEST/cfg" || exit 1
  ) || fail_msg "$FAIL_MSG. Failed to build cppcheck."

  mkdir "$DEST" || fail_msg "$FAIL_MSG. Failed to create '$DEST' directory."
  mkdir "$DEST/cfg" || fail_msg "$FAIL_MSG. Failed to create '$DEST/cfg' directory."

  cp "$TMP_DIR/$NAME/cppcheck" "$DEST" || fail_msg "$FAIL_MSG. Failed to copy cppcheck to '$DEST' directory."
  cp "$TMP_DIR/$NAME/cfg/std.cfg" "$DEST/cfg" || fail_msg "$FAIL_MSG. Failed to copy cfg/std.cfg to '$DEST/cfg' directory."

  remove_gitignore_files_at "$DEST"
  chmod -R u-w "$DEST" || fail_msg "$FAIL_MSG. Failed to remove write permission from '$DEST' directory contents."
}

function setup_vera() {
  NAME="$1"
  shift

  DEST=$(pwd)/"$1"
  shift

  URL="$1"
  shift

  CHECKSUM="$1"
  shift

  FAIL_MSG="Failed to setup '$NAME' prerequisite"

  if [ "$CLEAN_MODE" == "no" ]
  then
    echo "$CHECKSUM $NAME" >> $TMP_DIR/.prerequisites
    grep -q "^$CHECKSUM $NAME\$" $TMP_DIR/.prerequisites.prev && return 0

    echo "Setting up $NAME prerequisite"
  fi

  if [ -e "$DEST" ]
  then
    chmod -R u+w "$DEST" || fail_msg "$FAIL_MSG. Failed to add write permission to '$DEST' directory contents."
    rm -rf "$DEST" || fail_msg "$FAIL_MSG. Cannot remove '$DEST' directory."
  fi

  if [ "$CLEAN_MODE" == "yes" ]
  then
    return 0
  fi

  wget --no-check-certificate -O "$TMP_DIR/$NAME.tar.gz" "$URL" || fail_msg "$FAIL_MSG. Cannot download '$URL' archive."

  echo "$CHECKSUM  $TMP_DIR/$NAME.tar.gz" | $SHA256SUM --check || fail_msg "$FAIL_MSG. Archive's checksum doesn't match."

  tar xzvf "$TMP_DIR/$NAME.tar.gz" -C "$TMP_DIR" || fail_msg "$FAIL_MSG. Failed to unpack archive."

  (
    cd "$TMP_DIR/$NAME" || exit 1
    mkdir build || exit 1
    cd build || exit 1
    cmake .. -DCMAKE_INSTALL_PREFIX="$DEST" || exit 1
    make -j || exit 1
    make install || exit 1
  ) || fail_msg "$FAIL_MSG. Failed to build vera++ 1.2.1."

  remove_gitignore_files_at "$DEST"
  chmod -R u-w "$DEST" || fail_msg "$FAIL_MSG. Failed to remove write permission from '$DEST' directory contents."
}

HOST_OS=`uname -s`

if [ "$HOST_OS" == "Darwin" ]
then
  SHA256SUM="shasum -a 256"
  TMP_DIR=`mktemp -d -t jerryscript`
else
  SHA256SUM="sha256sum --strict"
  TMP_DIR=`mktemp -d --tmpdir=./`
fi

if [ "$CLEAN_MODE" == "yes" ]
then
  rm -f $PREREQUISITES_INSTALLED_LIST_FILE
else
  touch $PREREQUISITES_INSTALLED_LIST_FILE || fail_msg "Failed to create '$PREREQUISITES_INSTALLED_LIST_FILE'."
  mv $PREREQUISITES_INSTALLED_LIST_FILE $TMP_DIR/.prerequisites.prev
fi

setup_from_zip "stm32f3" \
               "./third-party/STM32F3-Discovery_FW_V1.1.0" \
               "http://www.st.com/st-web-ui/static/active/en/st_prod_software_internet/resource/technical/software/firmware/stm32f3discovery_fw.zip" \
               "cf81efd07d627adb58adc20653eecb415878b6585310b77b0ca54a34837b3855" \
               "STM32F3-Discovery_FW_V1.1.0/*"

setup_from_zip "stm32f4" \
               "./third-party/STM32F4-Discovery_FW_V1.1.0" \
               "http://www.st.com/st-web-ui/static/active/en/st_prod_software_internet/resource/technical/software/firmware/stsw-stm32068.zip" \
               "8e67f7b930c6c02bd7f89a266c8d1cae3b530510b7979fbfc0ee0d57e7f88b81" \
               "STM32F4-Discovery_FW_V1.1.0/*"

setup_cppcheck "cppcheck-1.69" \
               "./third-party/cppcheck" \
               "http://downloads.sourceforge.net/project/cppcheck/cppcheck/1.69/cppcheck-1.69.tar.bz2" \
               "4bd5c8031258ef29764a4c92666384238a625beecbb2aceeb7065ec388c7532e"

setup_vera "vera++-1.2.1" \
           "./third-party/vera++" \
           "https://bitbucket.org/verateam/vera/downloads/vera++-1.2.1.tar.gz" \
           "99b123c8f6d0f4fe9ee90397c461179066a36ed0d598d95e015baf2d3b56956b"

if [ "$CLEAN_MODE" == "no" ]
then
  mv $TMP_DIR/.prerequisites $PREREQUISITES_INSTALLED_LIST_FILE || fail_msg "Failed to write '$PREREQUISITES_INSTALLED_LIST_FILE'"
fi

clean_on_exit "OK"
