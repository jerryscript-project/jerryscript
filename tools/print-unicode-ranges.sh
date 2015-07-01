#!/bin/bash

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

#
# http://www.unicode.org/Public/3.0-Update/UnicodeData-3.0.0.txt
#
UNICODE_DATA_PATH="$1"

#
# One of unicode character category names (Lu, Ll, Nl, etc.)
#
UNICODE_CHAR_CATEGORY="$2"

UNICODE_CHAR_CATEGORY_UPPER_CASE=`echo $UNICODE_CHAR_CATEGORY | tr '[:lower:]' '[:upper:]'`

#
# 1. Print character codes, categories, and names
# 2. Filter by category
# 3. Print character codes and names without categories
# 4. Sort
# 5. Add '0x' to each line
# 6. Combine hexadecimal numbers into named ranges
# 7. Print ranges in format "LIT_UNICODE_RANGE_$UNICODE_CHAR_CATEGORY_UPPER_CASE (range_begin, range_end) /* range name */"
#

cut -d ';' "$UNICODE_DATA_PATH" -f 1,2,3 \
      | grep ";$UNICODE_CHAR_CATEGORY\$" \
      | cut -d ';' -f 1,2 \
      | sort \
      | awk 'BEGIN { FS=";"; OFS=";" } { print "0x"$1, $2; }' \
      | awk --non-decimal-data \
        'BEGIN \
         { \
           FS=";"; \
           OFS=";"; \
           is_in_range=0; \
         } \
         \
         function output_next_range () \
         { \
           if (range_begin == range_prev) \
           { \
             print range_begin, range_prev, range_begin_name; \
           } \
           else \
           { \
             print range_begin, range_prev,  range_begin_name, range_prev_name; \
           } \
         } \
         \
         { \
           if (is_in_range == 0) \
           { \
             is_in_range=1; \
             range_begin=$1; \
             range_prev=$1; \
             range_begin_name=$2; \
             range_prev_name=$2; \
           } \
           else \
           { \
             if (range_prev + 1 == $1) \
             { \
               range_prev=$1; \
               range_prev_name=$2
             } \
             else \
             { \
               output_next_range(); \
               range_begin=$1; \
               range_prev=$1; \
               range_begin_name=$2; \
               range_prev_name=$2; \
             } \
           } \
         } \
         \
         END \
         { \
           output_next_range(); \
         }' \
         | awk \
          'BEGIN \
           { \
             FS = ";" \
           } \
           { \
             range_string = sprintf ("LIT_UNICODE_RANGE_'$UNICODE_CHAR_CATEGORY_UPPER_CASE' (%s, %s)", $1, $2); \
             range_string_length = length (range_string); \
             \
             range_begin_name=$3; \
             range_end_name=$4; \
             \
             range_begin_name_length = length (range_begin_name); \
             range_end_name_length = length (range_end_name); \
             \
             printf "%s", range_string; \
             if (range_end_name_length == 0) \
             { \
               printf " /* %s */\n", range_begin_name; \
             } \
             else \
             { \
               if (range_begin_name_length > range_end_name_length) \
               { \
                 indent1 = 0; \
                 indent2 = range_string_length + range_begin_name_length / 2;
                 indent3 = range_string_length + (range_begin_name_length - range_end_name_length) / 2; \
               } \
               else \
               { \
                 indent1 = (range_end_name_length - range_begin_name_length) / 2; \
                 indent2 = range_string_length + range_end_name_length / 2;
                 indent3 = range_string_length; \
               } \
               indent3 = indent3 + 3; \
               fmt1 = sprintf (" /* %%%ds%%s\n", indent1); \
               fmt2 = sprintf (" %%%ds<--->\n", indent2); \
               fmt3 = sprintf (" %%%ds%%s */\n", indent3); \
               \
               printf fmt1, "", $3; \
               printf fmt2, ""; \
               printf fmt3, "", $4; \
             } \
             \
             printf "\n"; \
           }'
