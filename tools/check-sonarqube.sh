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

if [[ "${TRAVIS_REPO_SLUG}" == "jerryscript-project/jerryscript"
  && ${TRAVIS_BRANCH} == "master"
  && ${TRAVIS_EVENT_TYPE} == "push" ]]
then
  git fetch --unshallow
  build-wrapper-linux-x86-64 --out-dir bw-output \
    ./tools/build.py --error-messages=on \
                     --jerry-cmdline-snapshot=on \
                     --jerry-debugger=on \
                     --line-info=on \
                     --mem-stats=on \
                     --profile=es2015-subset \
                     --snapshot-save=on \
                     --snapshot-exec=on \
                     --valgrind=on \
                     --vm-exec-stop=on
  sonar-scanner -Dsonar.projectVersion="${TRAVIS_COMMIT}"
else
  # SonarQube analysis works only on the master branch.
  # Ensure the build works with the options used for the analysis.
  ./tools/build.py --error-messages=on \
                   --jerry-cmdline-snapshot=on \
                   --jerry-debugger=on \
                   --line-info=on \
                   --mem-stats=on \
                   --profile=es2015-subset \
                   --snapshot-save=on \
                   --snapshot-exec=on \
                   --valgrind=on \
                   --vm-exec-stop=on
fi
