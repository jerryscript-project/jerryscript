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
    set struct_marks {}
    set expect_struct_name false
    set prev_tok ""
    set def_start false
    set expect_newline false
    set check_newline true

    foreach token [getTokens $file_name 1 0 -1 -1 {}] {
        set tok_val [lindex $token 0]
        set line_num [lindex $token 1]
        set col_num [lindex $token 2]
        set tok_type [lindex $token 3]

        if {$state == "macro"} {
            if {$col_num == 0} {
                set state "normal"
            } else {
                continue
            }
        }

        if {$tok_type in {space ccomment cppcomment newline}} {
            continue
        }

        if {$tok_type == "pp_define"} {
            set state "macro"
            set prev_tok ""
            set def_start false
            continue
        }

        if {$expect_struct_name == true} {
            if {$tok_type == "identifier" && $line_num != [lindex $prev_tok 1]} {
                report $file_name $line_num "type name should be on the same line with the rightbrace"
            }
            set expect_struct_name false
        }

        # check that rightbrace and typename (in struct, union and enum definitons) are on the same line
        if {$tok_type in {struct enum union}} {
            set def_start true
        } elseif {$tok_type == "semicolon"} {
            set def_start false
        } elseif {$tok_type == "leftbrace"} {
            lappend cols $col_num
            lappend lines $line_num
            if {$def_start == true} {
                lappend struct_marks 1
                set def_start false
            } elseif {[lindex $prev_tok 3] == "assign"} {
                lappend struct_marks 2
                set check_newline false
            } else {
                lappend struct_marks 0
            }
        } elseif {$tok_type == "rightbrace"} {
            if {[llength $lines] > 0} {
                if {[lindex $struct_marks end] == 1} {
                    set expect_struct_name true
                    set check_newline false
                } elseif {[lindex $struct_marks end] == 2} {
                    set check_newline false
                }
                set lines [lreplace $lines end end]
                set cols [lreplace $cols end end]
                set struct_marks [lreplace $struct_marks end end]
            } else {
                report $file_name $line_num "unmatched brace"
            }
        }

        # check that braces are on separate lines
        if {$check_newline == true} {
            if {$expect_newline == true} {
                if {$tok_type == "semicolon"} {
                    # empty
                } elseif {[lindex $prev_tok 1] == $line_num} {
                    report $file_name $line_num "brace should be placed on a separate line"
                } else {
                    set expect_newline false
                }
            } elseif {$tok_type in {leftbrace rightbrace}} {
                if {[lindex $prev_tok 1] == $line_num} {
                    report $file_name $line_num "brace should be placed on a separate line"
                }
                set expect_newline true
            }
        } else {
            set check_newline true
        }

        set prev_tok $token
    }
}
