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

foreach fileName [getSourceFileNames] {
    set funcStart 0
    set funcName ""
    set lineNumber 1
    foreach line [getAllLines $fileName] {
        if {[regexp {^((static |const )*\w+ )*\w+ \(.*[,\)]} $line]} {
            set type {}
            set modifier {}
            if {$funcStart == 0} {
                regexp {^((static |const )*\w+ )*(\w+) \(} $line matched type modifier funcName
            }
        }

        if {[regexp {^\{$} $line]} {
            set funcStart 1
        }

        if {$funcStart == 1} {
            if {[regexp {^\}$} $line] && [string length $funcName] != 0} {
                report $fileName $lineNumber "missing comment at the end of function: /* $funcName */"
                set funcStart 0
            } elseif {[regexp {^\} /\*\s*\w+\s*\*/$} $line] && [string length $funcName] != 0} {
                set comment {}
                regexp {^\} /\*\s*(\w+)\s*\*/$} $line -> comment
                if {$comment != $funcName} {
                    report $fileName $lineNumber "comment missmatch. (Current: $comment, Expected: $funcName) "
                }
                set funcStart 0
            } elseif {[regexp {^\}.*;?$} $line]} {
                set funcStart 0
            }
        }

        incr lineNumber
    }
}
