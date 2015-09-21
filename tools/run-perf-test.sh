#!/bin/bash

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

trap "exit 2" INT

function pr_err() {
  echo -e "\e[91mError: $@\e[39m"
}

function exit_err() {
  pr_err $@

  exit 1
}

USAGE="Usage:\n    sudo run.sh OLD_ENGINE NEW_ENGINE REPEATS TIMEOUT BENCH_FOLDER"

if [ "$#" -ne 5 ]
then
  echo -e "${USAGE}"
  exit_err "Argument number mismatch..."
fi

ENGINE_OLD="$1"
ENGINE_NEW="$2"
REPEATS="$3"
TIMEOUT="$4"
BENCH_FOLDER="$5"

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
mem_rel_mult=1.0

function run-compare()
{
  COMMAND=$1
  PRE=$2
  TEST=$3

  OLD=$(timeout "${TIMEOUT}" ${COMMAND} "${ENGINE_OLD}" "${TEST}") || return 1
  NEW=$(timeout "${TIMEOUT}" ${COMMAND} "${ENGINE_NEW}" "${TEST}") || return 1

  #check result
  ! $OLD || ! $NEW || return 1

  #calc relative speedup
  rel=$(echo "${OLD}" "${NEW}" | awk '{print $2 / $1; }')

  #increment n
  ((${PRE}_n++))

  #accumulate relative speedup
  eval "rel_mult=\$${PRE}_rel_mult"
  rel_mult=$(echo "$rel_mult" "$rel" | awk '{print $1 * $2;}')
  eval "${PRE}_rel_mult=\$rel_mult"

  #calc percent to display
  percent=$(echo "$rel" | awk '{print (1.0 - $1) * 100; }')
  printf "%28s" "$(printf "%6s->%6s (%3.3f)" "$OLD" "$NEW" "$percent")"
}

function run-test()
{
  TEST=$1

  # print only filename
  printf "%40s | " "${TEST##*/}"
  run-compare "./tools/rss-measure.sh"      "mem"   "${TEST}" || return 1
  printf " | "
  run-compare "./tools/perf.sh ${REPEATS}"  "perf"  "${TEST}" || return 1
  printf " | "
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
printf "%40s | %28s | %28s |\n" "Benchmark" "RSS<br>(+ is better)" "Perf<br>(+ is better)"
printf "%40s | %28s | %28s |\n" "---------" "---" "----"

run-suite "${BENCH_FOLDER}"

mem_rel_gmean=$(echo "$mem_rel_mult" "$mem_n" | awk '{print $1 ^ (1.0 / $2);}')
mem_percent_gmean=$(echo "$mem_rel_gmean" | awk '{print (1.0 - $1) * 100;}')

perf_rel_gmean=$(echo "$perf_rel_mult" "$perf_n" | awk '{print $1 ^ (1.0 / $2);}')
perf_percent_gmean=$(echo "$perf_rel_gmean" | awk '{print (1.0 - $1) * 100;}')

printf "%40s | %28s | %28s |\n" "Geometric mean:" "RSS reduction: $mem_percent_gmean%" "Speed up: $perf_percent_gmean%"
date
