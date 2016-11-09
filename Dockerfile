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

FROM ubuntu:16.04

LABEL description="Ubuntu image to build and test JerryScript"

ENV JERRYSCRIPT_ROOT=/jerryscript

COPY / ${JERRYSCRIPT_ROOT}/

RUN apt-get update -q

# apt-get-install-deps.sh depends on sudo
RUN apt-get -q -y install \
    python \
    python-pip \
    sudo

RUN ${JERRYSCRIPT_ROOT}/tools/apt-get-install-deps.sh
RUN pip install pylint==1.6.5
