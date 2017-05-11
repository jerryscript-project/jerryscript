#!/bin/sh

# Copyright 2015-2016 Samsung Electronics Co., Ltd.
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

# hack to invalidate Docker cache
touch README.md

echo "-------"
echo "tagging: "
REPO_NAME=$(basename `git rev-parse --show-toplevel`)
GIT_REV=$(git rev-parse --short HEAD)

IMAGE_NAME="$REPO_NAME:$GIT_REV"
echo $IMAGE_NAME
echo "-------"

echo "---------"
echo "building: "
docker \
  build -t $IMAGE_NAME \
  -f ./targets/docker/Dockerfile \
  .

echo "---------"

echo "-------"
echo "running: "
docker run -t -i $IMAGE_NAME
echo "-------"