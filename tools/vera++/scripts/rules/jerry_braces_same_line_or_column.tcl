#!/usr/bin/tclsh

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

foreach file_name [getSourceFileNames] {
    set state "normal"
    set lines {}
    set cols {}

    foreach token [getTokens $file_name 1 0 -1 -1 {}] {
        set tok_val [lindex $token 0]
        set line_num [lindex $token 1]
        set col_num [lindex $token 2]
        set tok_type [lindex $token 3]

        if {$state == "macro"} {
            if {$col_num == 0} {
                set state "normal"
            } else {
                set prev_tok_line $line_num
                continue
            }
        }

        if {$tok_type in {space ccomment cppcomment newline}} {
            continue
        }

        if {$tok_type == "pp_define"} {
            set state "macro"
            continue
        }

        if {$tok_type == "leftbrace"} {
            lappend cols $col_num
            lappend lines $line_num
        } elseif {$tok_type == "rightbrace"} {
            if {[llength $lines] > 0} {
                if {[lindex $lines end] != $line_num && [lindex $cols end] != $col_num} {
                    report $file_name $line_num "matching braces should be on the same line or column"
                }
                set lines [lreplace $lines end end]
                set cols [lreplace $cols end end]
            } else {
                report $file_name $line_num "unmatched brace"
            }
        }
    }
}
