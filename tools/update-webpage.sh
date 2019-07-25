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

if [ $# -ne 1 ]; then
  echo "Please, specify your gh-pages clone directory: update-webpage <gh-pages clone dir>"
  exit 1
fi

gh_pages_dir=$1
docs_dir=`dirname $(readlink -f $0)`"/../docs"

GETTING_STARTED_MD="00.GETTING-STARTED.md"
CONFIGURATION_MD="01.CONFIGURATION.md"
API_REFERENCE_MD="02.API-REFERENCE.md"
API_EXAMPLES_MD="03.API-EXAMPLE.md"
INTERNALS_MD="04.INTERNALS.md"
PORT_API_MD="05.PORT-API.md"
REFERENCE_COUNTING_MD="06.REFERENCE-COUNTING.md"
DEBUGGER_MD="07.DEBUGGER.md"
CODING_STANDARDS_MD="08.CODING-STANDARDS.md"
EXT_REFERENCE_ARG_MD="09.EXT-REFERENCE-ARG.md"
EXT_REFERENCE_HANDLER_MD="10.EXT-REFERENCE-HANDLER.md"
EXT_REFERENCE_AUTORELEASE_MD="11.EXT-REFERENCE-AUTORELEASE.md"
EXT_REFERENCE_MODULE_MD="12.EXT-REFERENCE-MODULE.md"
DEBUGGER_TRANSPORT_MD="13.DEBUGGER-TRANSPORT.md"
EXT_REFERENCE_HANDLE_SCOPE_MD="14.EXT-REFERENCE-HANDLE-SCOPE.md"
MODULE_SYSTEM_MD="15.MODULE-SYSTEM.md"
MIGRATION_GUIDE_MD="16.MIGRATION-GUIDE.md"

declare -A titles

titles[$GETTING_STARTED_MD]="Getting Started"
titles[$CONFIGURATION_MD]="Configuration"
titles[$API_REFERENCE_MD]="API Reference"
titles[$API_EXAMPLES_MD]="API Examples"
titles[$INTERNALS_MD]="Internals"
titles[$PORT_API_MD]="Port API"
titles[$REFERENCE_COUNTING_MD]="Reference Counting"
titles[$DEBUGGER_MD]="Debugger"
titles[$CODING_STANDARDS_MD]="Coding Standards"
titles[$EXT_REFERENCE_ARG_MD]="'Extension API: Argument Validation'"
titles[$EXT_REFERENCE_HANDLER_MD]="'Extension API: External Function Handlers'"
titles[$EXT_REFERENCE_AUTORELEASE_MD]="'Extension API: Autorelease Values'"
titles[$EXT_REFERENCE_MODULE_MD]="'Extension API: Module Support'"
titles[$DEBUGGER_TRANSPORT_MD]="'Debugger Transport'"
titles[$EXT_REFERENCE_HANDLE_SCOPE_MD]="'Extension API: Handle Scope'"
titles[$MODULE_SYSTEM_MD]="'Module System (EcmaScript2015)'"
titles[$MIGRATION_GUIDE_MD]="Migration Guide"

for docfile in $docs_dir/*.md; do
  docfile_base=`basename $docfile`

  permalink=`echo $docfile_base | cut -d'.' -f 2 | tr '[:upper:]' '[:lower:]'`
  missing_title=`echo $permalink | tr '-' ' '`

  # the first three documents belong to the navigation bar
  category=$([[ $docfile_base =~ ^0[0-3] ]] && echo "navbar" || echo "documents")

  # generate appropriate header for each *.md
  echo "---"                                             >  $gh_pages_dir/$docfile_base
  echo "layout: page"                                    >> $gh_pages_dir/$docfile_base
  echo "title: ${titles[$docfile_base]:-$missing_title}" >> $gh_pages_dir/$docfile_base
  echo "category: ${category}"                           >> $gh_pages_dir/$docfile_base
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

    printf "%s%s%s\n", line_beg, link, line_end
  }
  ' $docfile                                             >> $gh_pages_dir/$docfile_base

  # fix image links
  sed -i -r -e 's/^!\[.*\]\(/&{{ site.github.url }}\//' $gh_pages_dir/$docfile_base
  sed -i -r -e 's/^!\[.*\]\(\{\{ site\.github\.url \}\}\/img.*$/&{: class="thumbnail center-block img-responsive" }/' $gh_pages_dir/$docfile_base

  # turn filenames into permalinks
  sed -i -r -e 's/docs\/[0-9]+\.(.*)\.md/\L\1/g' $gh_pages_dir/$docfile_base

  # replace span tags to div
  sed -i 's/<span class=/<div class=/g' $gh_pages_dir/$docfile_base
  sed -i 's/<\/span>/<\/div>/g' $gh_pages_dir/$docfile_base

  # remove table header separators
  sed -i '/^| ---/d' $gh_pages_dir/$docfile_base

  # update images
  cp -Ru $docs_dir/img $gh_pages_dir
done
