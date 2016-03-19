#!/usr/bin/tclsh

# Copyright 2016 Samsung Electronics Co., Ltd.
# Copyright 2016 University of Szeged.
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

proc check_part_of_the_file {file line_num col_start col_end} {
    if {$col_start == $col_end} {
        return
    }

    set line [getLine $file $line_num]
    set line [string range $line $col_start $col_end]

    if {[regexp {\w\*\s\w+} $line]
         || [regexp {\w\*\)} $line]
         || [regexp {\w\*$} $line]} {
        report $file $line_num "there should be a space between the referenced type and the pointer declarator."
    }
}

foreach fileName [getSourceFileNames] {
    set checkLine 1
    set checkColStart 0
    set seenOmitToken false
    foreach token [getTokens $fileName 1 0 -1 -1 {}] {
        set lineNumber [lindex $token 1]
        set colNumber [lindex $token 2]
        set tokenType [lindex $token 3]

        if {$checkLine != $lineNumber} {
            if {!$seenOmitToken} {
                check_part_of_the_file $fileName $checkLine $checkColStart end
            }
            set checkColStart $colNumber
            set checkLine $lineNumber
        } elseif {$seenOmitToken} {
            set checkColStart $colNumber
        }

        if {$tokenType in {ccomment cppcomment stringlit}} {
            check_part_of_the_file $fileName $checkLine $checkColStart $colNumber
            set seenOmitToken true
        } else {
            set seenOmitToken false
        }
    }
}
