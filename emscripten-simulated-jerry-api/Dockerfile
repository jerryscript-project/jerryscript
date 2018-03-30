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

FROM jerryscript

LABEL description="Ubuntu image to build and test JerryScript with Emscripten & Node.js"

RUN apt-get install -q -y \
    curl \
    # Install Java (optional, only needed for Closure Compiler minification)
    default-jre \
    # emsdk depends on git
    git

# Install Node 7.x
# 6.14.1 is the minimum for correct TypedArray support, but let's install
# a slightly newer one.
RUN curl -sL https://deb.nodesource.com/setup_7.x | sudo -E bash - && \
    sudo apt-get install -y nodejs

RUN cd ${JERRYSCRIPT_ROOT} && npm install weak

RUN ${JERRYSCRIPT_ROOT}/tools/prepare-emx.sh
