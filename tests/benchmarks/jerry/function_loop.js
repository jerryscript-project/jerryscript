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

var x = 7;
var y = 3;
var count = 1000000;

function cse_opt(x, y)
{
    var tmp1 = x * x;
    var tmp2 = y * y;
    var tmp3 = tmp1 * tmp1;
    var tmp4 = tmp2 * tmp2;
    
    for (var i = 0; i < count; i++) {
        var cached1 = tmp3 * x
        var cached2 = tmp4 * y;
        var cached_n_cached = cached1 + cached2;
        var ret_val = cached_n_cached + cached_n_cached;
    }
    
    return ret_val + ret_val;
};

cse_opt(x, y);