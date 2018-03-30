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

# This script installs emsdk and the specific SDK version that we are using.
# Important environment variables:
# - EMSDK: directory path where to put the emsdk folder (defaults to
#   $HOME/emsdk when the variable is not set)
# - JERRYSCRIPT_ROOT: directory path where the jerryscript repo is located.

EMSDK_URL=https://s3.amazonaws.com/mozilla-games/emscripten/releases/emsdk-portable.tar.gz
EMSDK_USE_VERSION=sdk-1.37.9-64bit

if [ -z "$EMSDK" ]; then
    EMSDK=$HOME/emsdk
    echo "No EMSDK env variable set, using default: $EMSDK"
fi

if [ -d "$EMSDK" ]; then
    echo "Existing SDK exists ($EMSDK), removing it..."
    rm -rf "$EMSDK"
fi

EMSDK_BIN="$EMSDK/emsdk"

EMSDK_PARENT_DIR=$(dirname "$EMSDK")
ARCHIVE_PATH=`mktemp`
echo "Downloading emsdk from $EMSDK_URL..."
curl -o "$ARCHIVE_PATH" $EMSDK_URL

echo "Extracting..."
TMP_TAR_DIR=`mktemp -d`
tar -xzf "$ARCHIVE_PATH" -C "$TMP_TAR_DIR"
mv "$TMP_TAR_DIR/emsdk-portable" "${EMSDK}"

echo "Cleaning up..."
rm "$ARCHIVE_PATH"
rm -rf "$TMP_TAR_DIR"

echo "Updating..."
"$EMSDK_BIN" update

echo "Installing $EMSDK_USE_VERSION..."
"$EMSDK_BIN" install $EMSDK_USE_VERSION

echo "Activating $EMSDK_USE_VERSION..."
"$EMSDK_BIN" activate $EMSDK_USE_VERSION

echo "Sourcing $EMSDK/emsdk_env.sh..."
source $EMSDK/emsdk_env.sh

echo "Jobs done."
