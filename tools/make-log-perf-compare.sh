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

commit_first=$1
shift

commit_second=$1
shift

exceptions="-e '' $*"

if [[ "$commit_first" == "" ]] || [[ "$commit_second" == "" ]]
then
  exit 1
fi

perf_first=`git notes --ref=arm-linux-perf show $commit_first | grep -v $exceptions`
if [ $? -ne 0 ]
then
  exit 1
fi

perf_second=`git notes --ref=arm-linux-perf show $commit_second | grep -v $exceptions`
if [ $? -ne 0 ]
then
  exit 1
fi

n=0
rel_mult=1.0
for bench in `echo "$perf_first" | cut -d ':' -f 1`
do
  value1=`echo "$perf_first" | grep "^$bench: " | cut -d ':' -f 2 | cut -d 's' -f 1`
  value2=`echo "$perf_second" | grep "^$bench: " | cut -d ':' -f 2 | cut -d 's' -f 1`
  rel=`echo $value1 $value2 | awk '{print $2 / $1; }'`
  percent=`echo $rel | awk '{print (1.0 - $1) * 100; }'`

  n=`echo $n | awk '{print $1 + 1;}'`;
  rel_mult=`echo $rel_mult $rel | awk '{print $1 * $2;}'`

  echo $bench":"$value1"s ->"$value2"s ("$percent" %)"
done

rel_gmean=`echo $rel_mult $n | awk '{print $1 ^ (1.0 / $2);}'`
percent_gmean=`echo $rel_gmean | awk '{print (1.0 - $1) * 100;}'`

echo
echo $n $rel_mult $rel_gmean "("$percent_gmean "%)"
