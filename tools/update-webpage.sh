#!/bin/bash

# Copyright 2015-2016 Samsung Electronics Co., Ltd.
# Copyright 2016 University of Szeged
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

if [ $# -ne 1 ]; then
  echo "Please, specify your gh-pages clone directory: update-webpage <gh-pages clone dir>"
  exit 1
fi

gh_pages_dir=$1
docs_dir=`dirname $(readlink -f $0)`"/../docs"

GETTING_STARTED_MD="01.GETTING-STARTED.md"
API_REFERENCE_MD="02.API-REFERENCE.md"
API_EXAMPLES_MD="03.API-EXAMPLE.md"
INTERNALS_MD="04.INTERNALS.md"
PORT_API_MD="05.PORT-API.md"

declare -A titles

titles[$GETTING_STARTED_MD]="Getting Started"
titles[$API_REFERENCE_MD]="API Reference"
titles[$API_EXAMPLES_MD]="API Examples"
titles[$INTERNALS_MD]="Internals"
titles[$PORT_API_MD]="Port API"

for docfile in $docs_dir/*.md; do
  docfile_base=`basename $docfile`

  permalink=`echo $docfile_base | cut -d'.' -f 2 | tr '[:upper:]' '[:lower:]'`
  missing_title=`echo $permalink | tr '-' ' '`

  # generate appropriate header for each *.md
  echo "---"                                             >  $gh_pages_dir/$docfile_base
  echo "layout: page"                                    >> $gh_pages_dir/$docfile_base
  echo "title: ${titles[$docfile_base]:-$missing_title}" >> $gh_pages_dir/$docfile_base
  echo "permalink: /$permalink/"                         >> $gh_pages_dir/$docfile_base
  echo "---"                                             >> $gh_pages_dir/$docfile_base
  echo                                                   >> $gh_pages_dir/$docfile_base
  echo "* toc"                                           >> $gh_pages_dir/$docfile_base
  echo "{:toc}"                                          >> $gh_pages_dir/$docfile_base
  echo                                                   >> $gh_pages_dir/$docfile_base

  # the file itself removing underscores inside links
  gawk \
  '
  !/\[.*\]\(#/ {
    print $0
  }

  /\[.*\]\(#/ {
    link_start_pos = index($0, "](#");
    line_beg = substr($0, 1, link_start_pos+2);
    line_remain = substr($0, link_start_pos+3);
    link_end_pos = index(line_remain, ")")
    link = substr(line_remain, 1, link_end_pos-1);
    line_end = substr(line_remain, link_end_pos)

    # delete underscores form the link
    gsub(/_/, "", link);

    printf "%s%s%s\n", line_beg, link, line_end
  }
  ' $docfile                                             >> $gh_pages_dir/$docfile_base

  # fix image links
  sed -i -r -e 's/^!\[.*\]\(/&{{ site.baseurl }}\//' $gh_pages_dir/$docfile_base
  sed -i -r -e 's/^!\[.*\]\(\{\{ site\.baseurl \}\}\/img.*$/&{: class="thumbnail center-block img-responsive" }/' $gh_pages_dir/$docfile_base

  # replace span tags to div
  sed -i 's/<span class=/<div class=/g' $gh_pages_dir/$docfile_base
  sed -i 's/<\/span>/<\/div>/g' $gh_pages_dir/$docfile_base

  # remove table header separators
  sed -i '/^| ---/d' $gh_pages_dir/$docfile_base

  # update images
  cp -Ru $docs_dir/img $gh_pages_dir
done
