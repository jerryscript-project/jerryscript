#!/usr/bin/tclsh

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

set maxEmptyLines 1

foreach f [getSourceFileNames] {
    set lineNumber 1
    set emptyCount 0
    set reported false
    foreach line [getAllLines $f] {
        if {[string trim $line] == ""} {
            incr emptyCount
            if {$emptyCount > $maxEmptyLines && $reported == "false"} {
                report $f $lineNumber "too many consecutive empty lines"
                set reported true
            }
        } else {
            set emptyCount 0
            set reported false
        }
        incr lineNumber
    }
}
