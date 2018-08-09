// Copyright JS Foundation and other contributors, http://js.foundation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

var src = "var a = 0; while(a) { switch(a) {" ;
for (var i = 0; i < 4000; i++)
    src += "-Infinity" + i + "\u00a0\u00a01.2e3";
src += "\udc00%f0%90%80%80\udc00";
print(src);
