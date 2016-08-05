#!/bin/bash

# Copyright 2014-2016 Samsung Electronics Co., Ltd.
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

trap "exit 2" INT

function pr_err() {
  echo -e "\e[91mError: $@\e[39m"
}

function exit_err() {
  pr_err $@

  exit 1
}

# Check if the specified build supports memory statistics options
function is_mem_stats_build() {
  [ -x "$1" ] || fail_msg "Engine '$1' is not executable"

  tmpfile=`mktemp`
  "$1" --mem-stats $tmpfile 2>&1 | grep -- "Ignoring JERRY_INIT_MEM_STATS flag because of !JMEM_STATS configuration." 2>&1 > /dev/null
  code=$?
  rm $tmpfile

  return $code
}

USAGE="Usage:\n    tools/run-perf-test.sh OLD_ENGINE NEW_ENGINE REPEATS TIMEOUT BENCH_FOLDER [-m result-file-name.md]"

if [ "$#" -lt 5 ]
then
  echo -e "${USAGE}"
  exit_err "Argument number mismatch..."
fi

ENGINE_OLD="$1"
ENGINE_NEW="$2"
REPEATS="$3"
TIMEOUT="$4"
BENCH_FOLDER="$5"
OUTPUT_FORMAT="$6"
OUTPUT_FILE="$7"

if [ "$#" -gt 5 ]
then
  if [ "${OUTPUT_FORMAT}" != "-m" ]
  then
    exit_err "Please, use '-m result-file-name.md' as last arguments"
  fi
  if [ -z "${OUTPUT_FILE}" ]
  then
    exit_err "Missing md file name. Please, define the filename. Ex.: '-m result-file-name.md'"
  fi

  rm -rf "${OUTPUT_FILE}"
fi

if [ "${REPEATS}" -lt 1 ]
then
  exit_err "REPEATS must be greater than 0"
fi

if [ "${TIMEOUT}" -lt 1 ]
then
  exit_err "TIMEOUT must be greater than 0"
fi

perf_n=0
mem_n=0

perf_rel_mult=1.0
perf_rel_inaccuracy_tmp=0
mem_rel_mult=1.0
mem_rel_inaccuracy_tmp="-1"

# Unicode "figure space" character
FIGURE_SPACE=$(echo -e -n "\xE2\x80\x87")

# Unicode "approximately equal" character
APPROXIMATELY_EQUAL=$(echo -n -e "\xE2\x89\x88")

function run-compare()
{
  COMMAND=$1
  PRE=$2
  TEST=$3
  PRECISION=$4
  UNIT=$5

  ABS_FP_FMT="%$((PRECISION + 4)).$((PRECISION))f$UNIT"
  REL_FP_FMT="%0.3f"
  REL_SHOW_PLUS_SIGN_FP_FMT="%+0.3f"

  OLD=$(timeout "${TIMEOUT}" ${COMMAND} "${ENGINE_OLD}" "${TEST}") || return 1
  NEW=$(timeout "${TIMEOUT}" ${COMMAND} "${ENGINE_NEW}" "${TEST}") || return 1

  #check result
  ! $OLD || ! $NEW || return 1

  OLD_value=$(echo "$OLD " | cut -d ' ' -f 1)
  OLD_inaccuracy=$(echo "$OLD " | cut -d ' ' -f 2)

  NEW_value=$(echo "$NEW " | cut -d ' ' -f 1)
  NEW_inaccuracy=$(echo "$NEW " | cut -d ' ' -f 2)

  #calc relative speedup
  eval "rel_mult=\$${PRE}_rel_mult"

  rel=$(echo "${OLD_value}" "${NEW_value}" | awk '{ print $2 / $1; }')

  #increment n
  ((${PRE}_n++))

  #calc percent to display
  PERCENT=$(echo "$rel" | awk '{print (1.0 - $1) * 100; }')

  if [[ "$OLD_inaccuracy" != "" && "$NEW_inaccuracy" != "" ]]
  then
    DIFF=$(printf "$ABS_FP_FMT -> $ABS_FP_FMT" $OLD_value $NEW_value)
    rel_inaccuracy=$(echo "$OLD_value $OLD_inaccuracy $NEW_value $NEW_inaccuracy" | \
                     awk "{
                            OLD_value=\$1
                            OLD_inaccuracy=\$2
                            NEW_value=\$3
                            NEW_inaccuracy=\$4

                            rel_inaccuracy = (NEW_value / OLD_value) * sqrt ((OLD_inaccuracy / OLD_value) ^ 2 + (NEW_inaccuracy / NEW_value) ^ 2)
                            if (rel_inaccuracy < 0) {
                              rel_inaccuracy = -rel_inaccuracy
                            }

                            print rel_inaccuracy
                          }")
    PERCENT_inaccuracy=$(echo "$rel_inaccuracy" | awk '{ print $1 * 100.0 }')

    ext=$(echo "$PERCENT $PERCENT_inaccuracy" | \
                awk "{
                       PERCENT=\$1
                       PERCENT_inaccuracy=\$2

                       if (PERCENT > 0.0 && PERCENT > PERCENT_inaccuracy) {
                         print \"[+]\"
                       } else if (PERCENT < 0 && -PERCENT > PERCENT_inaccuracy) {
                         print \"[-]\"
                       } else {
                         print \"[$APPROXIMATELY_EQUAL]\"
                       }
                     }")

    if [[ $rel_inaccuracy_tmp -lt 0 ]]
    then
      return 1
    fi

    eval "rel_inaccuracy_tmp=\$${PRE}_rel_inaccuracy_tmp"

    rel_inaccuracy_tmp=$(echo "$rel $rel_inaccuracy $rel_inaccuracy_tmp" | \
                         awk "{
                                rel=\$1
                                rel_inaccuracy=\$2
                                rel_inaccuracy_tmp=\$3
                                print rel_inaccuracy_tmp + (rel_inaccuracy / rel) ^ 2
                              }")

    eval "${PRE}_rel_inaccuracy_tmp=\$rel_inaccuracy_tmp"

    PERCENT=$(printf "%8s %11s" $(printf "$REL_SHOW_PLUS_SIGN_FP_FMT%%" $PERCENT) $(printf "(+-$REL_FP_FMT%%)" $PERCENT_inaccuracy))
    PERCENT="$PERCENT : $ext"

    if [ "${OUTPUT_FORMAT}" == "-m" ]
    then
      WIDTH=42
      MD_DIFF=$(printf "%s%s" "$DIFF" "$(printf "%$(($WIDTH - ${#DIFF}))s")")
      MD_PERCENT=$(printf "%s%s" "$(printf "%$(($WIDTH - ${#PERCENT}))s")" "$PERCENT")

      MD_FORMAT="\`%s\`<br>\`%s\`"
    fi

    CONSOLE_FORMAT="%20s : %19s"
  else
    ext=""

    if [[ "$OLD_inaccuracy" != "" || "$NEW_inaccuracy" != "" ]]
    then
      return 1;
    fi

    DIFF=$(printf "$ABS_FP_FMT -> $ABS_FP_FMT" $OLD_value $NEW_value)
    PERCENT=$(printf "$REL_SHOW_PLUS_SIGN_FP_FMT%%" $PERCENT)

    if [ "${OUTPUT_FORMAT}" == "-m" ]
    then
      WIDTH=20
      MD_DIFF=$(printf "%s%s" "$DIFF" "$(printf "%$(($WIDTH - ${#DIFF}))s")")
      MD_PERCENT=$(printf "%s%s" "$(printf "%$(($WIDTH - ${#PERCENT}))s")" "$PERCENT")

      MD_FORMAT="\`%s\`<br>\`%s\`"
    fi

    CONSOLE_FORMAT="%14s : %8s"
  fi

  rel_mult=$(echo "$rel_mult" "$rel" | awk '{print $1 * $2;}')

  eval "${PRE}_rel_mult=\$rel_mult"

  if [ "${OUTPUT_FORMAT}" == "-m" ]
  then
    printf "$MD_FORMAT" "$MD_DIFF" "$MD_PERCENT" | sed "s/ /$FIGURE_SPACE/g" >> "${OUTPUT_FILE}"
  fi

  printf "$CONSOLE_FORMAT" "$DIFF" "$PERCENT"
}

function run-test()
{
  TEST=$1

  # print only filename
  if [ "${OUTPUT_FORMAT}" == "-m" ]
  then
    printf "%s | " "${TEST##*/}" >> "${OUTPUT_FILE}"
  fi

  printf "%50s | " "${TEST##*/}"

  if [ "$IS_MEM_STAT" -ne 0 ]
  then
    run-compare "./tools/mem-stats-measure.sh"      "mem"   "${TEST}" 0   || return 1
  else
    run-compare "./tools/rss-measure.sh"      "mem"   "${TEST}" 0 k || return 1
  fi

  if [ "${OUTPUT_FORMAT}" == "-m" ]
  then
    printf " | " >> "${OUTPUT_FILE}"
  fi

  printf " | "
  run-compare "./tools/perf.sh ${REPEATS}"  "perf"  "${TEST}" 3 s || return 1

  if [ "${OUTPUT_FORMAT}" == "-m" ]
  then
    printf "\n" >> "${OUTPUT_FILE}"
  fi

  printf "\n"
}

function run-suite()
{
  FOLDER=$1

  for BENCHMARK in ${FOLDER}/*.js
  do
    run-test "${BENCHMARK}" 2> /dev/null || printf "<FAILED>\n" "${BENCHMARK}";
  done
}

date

is_mem_stats_build "${ENGINE_OLD}" || is_mem_stats_build "${ENGINE_NEW}"
IS_MEM_STAT=$?

if [ "${OUTPUT_FORMAT}" == "-m" ]
then
  if [ "$IS_MEM_STAT" -ne 0 ]
  then
    echo "Benchmark | Peak alloc.<br>(+ is better) | Perf<br>(+ is better)" >> "${OUTPUT_FILE}"
  else
    echo "Benchmark | RSS<br>(+ is better) | Perf<br>(+ is better)" >> "${OUTPUT_FILE}"
  fi
  echo "---------: | --------- | ---------" >> "${OUTPUT_FILE}"
fi

if [ "$IS_MEM_STAT" -ne 0 ]
then
  printf "%50s | %25s | %35s\n" "Benchmark" "Peak alloc.(+ is better)" "Perf(+ is better)"
else
  printf "%50s | %25s | %35s\n" "Benchmark" "RSS(+ is better)" "Perf(+ is better)"
fi

run-suite "${BENCH_FOLDER}"

mem_rel_gmean=$(echo "$mem_rel_mult" "$mem_n" | awk '{print $1 ^ (1.0 / $2);}')
mem_percent_gmean=$(echo "$mem_rel_gmean" | awk '{print (1.0 - $1) * 100;}')
if [[ $mem_rel_inaccuracy_tmp != "-1" ]]
then
  exit_err "Incorrect inaccuracy calculation for memory consumption geometric mean"
fi

perf_rel_gmean=$(echo "$perf_rel_mult" "$perf_n" | awk '{print $1 ^ (1.0 / $2);}')
perf_percent_gmean=$(echo "$perf_rel_gmean" | awk '{print (1.0 - $1) * 100;}')
if [[ "$perf_rel_inaccuracy_tmp" == "-1" ]]
then
  exit_err "Incorrect inaccuracy calculation for performance geometric mean"
else
  perf_percent_inaccuracy=$(echo "$perf_rel_gmean $perf_rel_inaccuracy_tmp $perf_n" | \
                            awk "{
                                   perf_rel_gmean=\$1
                                   perf_rel_inaccuracy_tmp=\$2
                                   perf_n=\$3

                                   print 100.0 * (perf_rel_gmean ^ (1.0 / perf_n) * sqrt (perf_rel_inaccuracy_tmp) / perf_n)
                                 }")
  perf_ext=$(echo "$perf_percent_gmean $perf_percent_inaccuracy" | \
             awk "{
                    perf_percent_gmean=\$1
                    perf_percent_inaccuracy=\$2

                    if (perf_percent_gmean > 0.0 && perf_percent_gmean > perf_percent_inaccuracy) {
                      print \"[+]\"
                    } else if (perf_percent_gmean < 0 && -perf_percent_gmean > perf_percent_inaccuracy) {
                      print \"[-]\"
                    } else {
                      print \"[$APPROXIMATELY_EQUAL]\"
                    }
                  }")
  perf_percent_inaccuracy=$(printf "(+-%0.3f%%) : $perf_ext" $perf_percent_inaccuracy)
fi

gmean_label_text="Geometric mean:"

if [ "${OUTPUT_FORMAT}" == "-m" ]
then
  mem_percent_gmean_text=$(printf "RSS reduction: \`%0.3f%%\`" "$mem_percent_gmean")
  perf_percent_gmean_text=$(printf "Speed up: \`%0.3f%% %s\`" "$perf_percent_gmean" "$perf_percent_inaccuracy")
  printf "%s | %s | %s\n" "$gmean_label_text" "$mem_percent_gmean_text" "$perf_percent_gmean_text" >> "${OUTPUT_FILE}"
fi

mem_percent_gmean_text=$(printf "RSS reduction: %0.3f%%" "$mem_percent_gmean")
perf_percent_gmean_text=$(printf "Speed up: %0.3f%% %s" "$perf_percent_gmean" "$perf_percent_inaccuracy")
printf "%50s | %25s | %51s\n" "$gmean_label_text" "$mem_percent_gmean_text" "$perf_percent_gmean_text"

date
