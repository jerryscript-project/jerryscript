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

foreach file_name [getSourceFileNames] {
    set state "control"
    set do_marks {}
    set prev_tok_type ""
    set prev_ctrl ""
    set expect_while false
    set expect_open_brace false
    set paren_count 0

    foreach token [getTokens $file_name 1 0 -1 -1 {if do while else for leftparen rightparen semicolon leftbrace rightbrace}] {
        set tok_val [lindex $token 0]
        set line_num [lindex $token 1]
        set col_num [lindex $token 2]
        set tok_type [lindex $token 3]

        if {$state == "expression"} {
            # puts "expression $paren_count $tok_type ($line_num , $col_num)"
            if {$tok_type == "leftparen"} {
                incr paren_count
            } elseif {$tok_type == "rightparen"} {
                incr paren_count -1
                if {$paren_count == 0} {
                    set state "control"
                    set expect_open_brace true
                } elseif {$paren_count < 0 } {
                    report $file_name $line_num "unexpected right parentheses"
                }
            } elseif {$tok_type != "semicolon"} {
                report $file_name $line_num "unexpected token: $tok_type"
            }
        } else {
            if {$expect_open_brace == true} {
                if {$tok_type == "if" && $prev_tok_type == "else"} {
                    # empty
                } elseif {$tok_type != "leftbrace"} {
                    report $file_name [lindex $prev_ctrl 1] "brace after \'[lindex $prev_ctrl 3]\' required"
                }
                set expect_open_brace false
            }

            if {$tok_type == "while" && ($expect_while == true || [lindex $prev_ctrl 3] == "do")} {
                set expect_while false
                set prev_ctrl ""
            } elseif {$tok_type in {if for while}} {
                set state "expression"
                set prev_ctrl $token
            } elseif {$tok_type in {do else}} {
                set expect_open_brace true
                set prev_ctrl $token
            } elseif {$tok_type == "leftbrace"} {
                if {[lindex $prev_ctrl 3] == "do"} {
                    lappend do_marks 1
                } else {
                    lappend do_marks 0
                }
                set prev_ctrl ""
            } elseif {$tok_type == "rightbrace"} {
                if {[llength $do_marks] > 0} {
                    if {[lindex $do_marks end] == 1} {
                        set expect_while true
                    }
                    set do_marks [lreplace $do_marks end end]
                } else {
                    report $file_name $line_num "unmatched brace"
                }
            }
        }

        set prev_tok_type $tok_type
    }
}
