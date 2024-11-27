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

MAGIC_STRINGS_GEN="tools/gen-strings.py"
MAGIC_STRINGS_INC_H="jerry-core/lit/lit-magic-strings.inc.h"
ECMA_ERROR_MESSAGES_INC_H="jerry-core/ecma/base/ecma-error-messages.inc.h"
PARSER_ERROR_MESSAGES_INC_H="jerry-core/parser/js/parser-error-messages.inc.h"
MAGIC_STRINGS_TEMP=$(mktemp lit-magic-strings.inc.h.XXXXXXXXXX)
ECMA_ERROR_MESSAGES_TEMP=$(mktemp ecma-error-messages.inc.h.XXXXXXXXXX)
PARSER_ERROR_MESSAGES_TEMP=$(mktemp parser-error-messages.inc.h.XXXXXXXXXX)

cp $MAGIC_STRINGS_INC_H "$MAGIC_STRINGS_TEMP"
cp $ECMA_ERROR_MESSAGES_INC_H "$ECMA_ERROR_MESSAGES_TEMP"
cp $PARSER_ERROR_MESSAGES_INC_H "$PARSER_ERROR_MESSAGES_TEMP"
$MAGIC_STRINGS_GEN
DIFF_RESULT=$?

if [ $DIFF_RESULT -eq 0 ]
then
  diff -q $MAGIC_STRINGS_INC_H "$MAGIC_STRINGS_TEMP"
  DIFF_RESULT=$?
  if [ $DIFF_RESULT -ne 0 ]
  then
    echo -e "\e[1;33m$MAGIC_STRINGS_INC_H must be re-generated. Run $MAGIC_STRINGS_GEN\e[0m"
    exit $DIFF_RESULT
  fi
  diff -q $ECMA_ERROR_MESSAGES_INC_H "$ECMA_ERROR_MESSAGES_TEMP"
  DIFF_RESULT=$?
  if [ $DIFF_RESULT -ne 0 ]
  then
    echo -e "\e[1;33m$ECMA_ERROR_MESSAGES_INC_H must be re-generated. Run $MAGIC_STRINGS_GEN\e[0m"
    exit $DIFF_RESULT
  fi
  diff -q $PARSER_ERROR_MESSAGES_INC_H "$PARSER_ERROR_MESSAGES_TEMP"
  DIFF_RESULT=$?
  if [ $DIFF_RESULT -ne 0 ]
  then
    echo -e "\e[1;33m$PARSER_ERROR_MESSAGES_INC_H must be re-generated. Run $MAGIC_STRINGS_GEN\e[0m"
    exit $DIFF_RESULT
  fi
fi
mv "$MAGIC_STRINGS_TEMP" $MAGIC_STRINGS_INC_H
mv "$ECMA_ERROR_MESSAGES_TEMP" $ECMA_ERROR_MESSAGES_INC_H
mv "$PARSER_ERROR_MESSAGES_TEMP" $PARSER_ERROR_MESSAGES_INC_H

exit $DIFF_RESULT
