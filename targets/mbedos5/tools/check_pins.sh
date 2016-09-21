#!/bin/bash

# Copyright (c) 2016 ARM Limited.
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

# If we're checking only for global variable definitions of pins, then
# file ordering doesn't matter. This is because:
#
# var a = b;
# var b = 7;
#
# will be accepted by jshint, just 'a' will evaluate to 'undefined'.
# Awkward, but at least it means we can have pins.js included at any
# point in the clump of files and it won't give us false positives.

cat js/*.js | jshint -c tools/jshint.conf - | grep "not defined"
if [ "$?" == 0 ]; then
    exit 1
fi
exit 0
