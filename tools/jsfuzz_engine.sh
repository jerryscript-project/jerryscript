# Copyright 2014 Samsung Electronics Co., Ltd.
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


# A simple harness that downloads and runs 'jsfunfuzz' against d8. This
# takes a long time because it runs many iterations and is intended for
# automated usage. The package containing 'jsfunfuzz' can be found as an
# attachment to this bug:
# https://bugzilla.mozilla.org/show_bug.cgi?id=jsfunfuzz

JSFUNFUZZ_URL="https://bugzilla.mozilla.org/attachment.cgi?id=310631"
JSFUNFUZZ_MD5="d0e497201c5cd7bffbb1cdc1574f4e32"

engine=$1

jsfunfuzz_file="./tools/jsfunfuzz.zip"
if [ ! -f "$jsfunfuzz_file" ]; then
  echo "Downloading $jsfunfuzz_file ..."
  wget -q -O "$jsfunfuzz_file" $JSFUNFUZZ_URL || exit 1
fi

jsfunfuzz_sum=$(md5sum "$jsfunfuzz_file" | awk '{ print $1 }')
if [ $jsfunfuzz_sum != $JSFUNFUZZ_MD5 ]; then
  echo "Failed to verify checksum!"
  exit 1
fi

jsfunfuzz_dir="./tools/jsfunfuzz"
if [ ! -d "$jsfunfuzz_dir" ]; then
  echo "Unpacking into $jsfunfuzz_dir ..."
  unzip "$jsfunfuzz_file" -d "$jsfunfuzz_dir" || exit 1
  echo "Patching runner ..."
  cat << EOF | patch -s -p0 -d "."
--- tools/jsfunfuzz/jsfunfuzz/multi_timed_run.py~
+++ tools/jsfunfuzz/jsfunfuzz/multi_timed_run.py
@@ -125,7 +125,7 @@
 
 def many_timed_runs():
     iteration = 0
-    while True:
+    while iteration < 100:
         iteration += 1
         logfilename = "w%d" % iteration
         one_timed_run(logfilename)
EOF
fi

python -u "$jsfunfuzz_dir/jsfunfuzz/multi_timed_run.py" 300 \
    $engine "$jsfunfuzz_dir/jsfunfuzz/jsfunfuzz.js"
exit_code=$(cat w* | grep " looking good" -c)
exit_code=$((100-exit_code))
tar -cjf fuzz-results-$(date +%y%m%d).tar.bz2 err-* w*
rm -f err-* w*

echo "Total failures: $exit_code"
exit $exit_code
