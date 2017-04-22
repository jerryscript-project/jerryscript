#!/usr/bin/env python

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

import argparse
import os
import webbrowser


def main():
    parser = argparse.ArgumentParser(description="JerryScript debugger client in HTML")
    parser.add_argument("address", action="store", nargs="?", default="localhost:5001",
                        help="specify a unique network address for connection (default: %(default)s)")
    args = parser.parse_args()

    webbrowser.open_new("file://%s?address=%s" % (os.path.abspath("jerry-client-ws.html"), args.address))


if __name__ == "__main__":
    main()
