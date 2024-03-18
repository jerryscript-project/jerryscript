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

var iter = 100;
async function f0() {
    iter--;
    function f6() {
        return f0;
    }
    var proxy_handler = {
        "get": f0,
    };

    f0.__proto__ = new Proxy(f6, proxy_handler);

    if ((iter >= 0)) {
        var v12 = f0();
    }
    return f0;
}
f0();
