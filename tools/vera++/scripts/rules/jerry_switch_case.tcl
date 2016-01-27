#!/usr/bin/tclsh

# Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

# switch-case

foreach fileName [getSourceFileNames] {
    set is_in_comment "no"
    set is_in_pp_define "no"

    foreach token [getTokens $fileName 1 0 -1 -1 {}] {
        set type [lindex $token 3]
        set lineNumber [lindex $token 1]

        if {$is_in_comment == "yes"} {
            set is_in_comment "no"
        }

        if {$type == "newline"} {
            set is_in_pp_define "no"
        } elseif {$type == "ccomment"} {
            set is_in_comment "yes"
        } elseif {[string first "pp_" $type] == 0} {
            if {$type == "pp_define"} {
                set is_in_pp_define "yes"
            }
        } elseif {$type == "space"} {
        } elseif {$type != "eof"} {
            if {$is_in_pp_define == "no" && $type == "switch"} {
                set next_token_start [lindex $token 2]
                incr next_token_start 1
                set line_num 0
                set state "switch"
                set case_block "no"
                set seen_braces 0
                foreach next_token [getTokens $fileName $lineNumber $next_token_start -1 -1 {}] {
                    set next_token_type [lindex $next_token 3]
                    set next_token_value [lindex $next_token 0]
                    if {$state == "switch"} {
                        if {$next_token_type == "ccomment" || $next_token_type == "space" || $next_token_type == "newline"} {
                            continue
                        } elseif {$next_token_type == "leftbrace"} {
                            set state "first-case"
                            continue
                        } else {
                            # TODO: check switch
                            continue
                        }
                    } elseif {$state == "first-case"} {
                        if {$next_token_type == "ccomment" || $next_token_type == "space" || $next_token_type == "newline"} {
                            continue
                        } elseif {$next_token_type == "case"} {
                            set state "case"
                            continue
                        } elseif {$next_token_type == "default"} {
                            set state "default"
                            continue
                        } else {
                            # Macros magic: give up
                            break
                        }
                    } elseif {$state == "case"} {
                        if {$next_token_type == "space"} {
                            set state "space-after-case"
                            continue
                        } else {
                            report $fileName [lindex $next_token 1] "There should be single space character after 'case' keyword (state $state)"
                        }
                    } elseif {$state == "space-after-case"} {
                        if {$next_token_type != "identifier" && $next_token_type != "intlit" && $next_token_type != "charlit" && $next_token_type != "sizeof"} {
                            report $fileName [lindex $next_token 1] "There should be single space character after 'case' keyword (state $state, next_token_type $next_token_type)"
                        } else {
                            set state "case-label"
                            continue
                        }
                    } elseif {$state == "case-label" || $state == "default"} {
                        set case_block "no"
                        if {$next_token_type != "colon"} {
                            continue
                        } else {
                            set state "colon"
                            continue
                        }
                    } elseif {$state == "after-colon-preprocessor"} {
                      if {$next_token_type == "newline"} {
                          set state "colon"
                      }
                    } elseif {$state == "colon"} {
                        if {$next_token_type == "space" || $next_token_type == "newline"} {
                            continue
                        } elseif {$next_token_type == "ccomment"} {
                            if {[string match "*FALL*" $next_token_value]} {
                                set state "fallthru"
                                set line_num [lindex $next_token 1]
                                continue
                            } else {
                                continue
                            }
                        } elseif {$next_token_type == "case"} {
                            set state "case"
                            continue
                        } elseif {$next_token_type == "default"} {
                            set state "default"
                            continue
                        } elseif {$next_token_type == "leftbrace"} {
                            set case_block "yes"
                            set state "wait-for-break"
                            continue
                        } elseif {$next_token_type == "identifier"} {
                            if {[string compare "JERRY_UNREACHABLE" $next_token_value] == 0
                                || [string first "JERRY_UNIMPLEMENTED" $next_token_value] == 0} {
                                set state "wait-for-semicolon"
                                continue
                            } else {
                                set state "wait-for-break"
                                continue
                            }
                        } elseif {$next_token_type == "break"
                                  || $next_token_type == "continue"
                                  || $next_token_type == "return"} {
                            set state "wait-for-semicolon"
                            continue
                        } elseif {[string first "pp_" $next_token_type] == 0} {
                            set state "after-colon-preprocessor"
                        } else {
                            set state "wait-for-break"
                            continue
                        }
                    } elseif {$state == "wait-for-semicolon"} {
                        if {$next_token_type == "semicolon"} {
                            set state "break"
                        }
                        continue
                    } elseif {$state == "wait-for-break"} {
                        if {$next_token_type == "case" || $next_token_type == "default"} {
                            report $fileName [lindex $next_token 1] "Missing break, continue or FALLTHRU comment before case (state $state)"
                        } elseif {$next_token_type == "leftbrace"} {
                            set state "inside-braces"
                            incr seen_braces 1
                            continue
                        } elseif {$next_token_type == "rightbrace"} {
                            if {$case_block == "yes"} {
                                set state "case-blocks-end"
                                continue
                            } else {
                                break
                            }
                        } elseif {[string compare "JERRY_UNREACHABLE" $next_token_value] == 0
                                   || [string first "JERRY_UNIMPLEMENTED" $next_token_value] == 0} {
                            set state "wait-for-semicolon"
                            continue
                        } elseif {$next_token_type == "ccomment" && [string match "*FALL*" $next_token_value]} {
                            set state "fallthru"
                            set line_num [lindex $next_token 1]
                            continue
                        } elseif {$next_token_type == "break"
                                  || $next_token_type == "continue"
                                  || $next_token_type == "return"
                                  || $next_token_type == "goto"} {
                            set state "wait-for-semicolon"
                            continue
                        }
                        continue
                    } elseif {$state == "break" || $state == "fallthru"} {
                        if {$case_block == "no"} {
                            if {$next_token_type == "ccomment" || $next_token_type == "space" || $next_token_type == "newline"} {
                                continue
                            } elseif {$next_token_type == "case"} {
                                set state "case"
                                continue
                            } elseif {$next_token_type == "default"} {
                                set state "default"
                                continue
                            } elseif {$next_token_type == "leftbrace"} {
                                set state "inside-braces"
                                incr seen_braces 1
                                continue
                            } elseif {$next_token_type == "rightbrace"} {
                                lappend switch_ends [lindex $next_token 1]
                                break
                            } elseif {$next_token_type == "break"
                                      || $next_token_type == "continue"
                                      || $next_token_type == "return"} {
                                set state "wait-for-semicolon"
                                continue
                            } else {
                                set state "wait-for-break"
                                continue
                            }
                        } else {
                            if {$next_token_type == "ccomment" || $next_token_type == "space" || $next_token_type == "newline"} {
                                continue
                            } elseif {$next_token_type == "case"} {
                                set state "case"
                                continue
                            } elseif {$next_token_type == "default"} {
                                set state "default"
                                continue
                            } elseif {$next_token_type == "leftbrace"} {
                                set state "inside-braces"
                                incr seen_braces 1
                                continue
                            } elseif {$next_token_type == "rightbrace"} {
                                set state "after-rightbrace"
                                continue
                            } elseif {$next_token_type == "break"
                                      || $next_token_type == "continue"
                                      || $next_token_type == "return"} {
                                set state "wait-for-semicolon"
                                continue
                            } else {
                                set state "wait-for-break"
                                continue
                            }
                        }
                    } elseif {$state == "inside-braces"} {
                        if {$next_token_type == "rightbrace"} {
                            incr seen_braces -1
                            if {$seen_braces == 0} {
                                set state "wait-for-break"
                                continue
                            }
                        } elseif {$next_token_type == "leftbrace"} {
                            incr seen_braces 1
                        }
                        continue
                    } elseif {$state == "after-rightbrace-preprocessor"} {
                        if {$next_token_type == "newline"} {
                            set state "after-rightbrace"
                        }
                   } elseif {$state == "after-rightbrace"} {
                        if {$next_token_type == "ccomment" || $next_token_type == "space" || $next_token_type == "newline"} {
                            continue
                        } elseif {$next_token_type == "case"} {
                            set state "case"
                            continue
                        } elseif {$next_token_type == "default"} {
                            set state "default"
                            continue
                        }  elseif {$next_token_type == "rightbrace"} {
                            lappend switch_ends [lindex $next_token 1]
                            break
                        } elseif {[string first "pp_" $next_token_type] == 0} {
                            set state "after-rightbrace-preprocessor"
                        } else {
                            report $fileName [lindex $next_token 1] "There should be 'case' or 'default' (state $state)"
                        }
                    } elseif {$state == "case-blocks-end-preprocessor"} {
                      if {$next_token_type == "newline"} {
                          set state "case-blocks-end"
                      }
                    } elseif {$state == "case-blocks-end"} {
                        if {$next_token_type == "ccomment" || $next_token_type == "space" || $next_token_type == "newline"} {
                            continue
                        } elseif {$next_token_type == "rightbrace"} {
                            lappend switch_ends [lindex $next_token 1]
                            break
                        } elseif {[string first "pp_" $next_token_type] == 0} {
                            set state "case-blocks-end-preprocessor"
                        } else {
                            report $fileName [lindex $next_token 1] "Missing break, continue or FALLTHRU comment before rightbrace (state $state)"
                        }
                    } else {
                        report $fileName [lindex $next_token 1] "Unknown state: $state"
                    }
                }
            }
        }
    }
}
