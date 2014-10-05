// Copyright 2014 Samsung Electronics Co., Ltd.
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

/*
<test file="12.02.01-001.js">
    <author>s.ryabkova</author>
    <checks>
        Check that in non strict mode variable with name "eval" could be created
    </checks>
    <keywords>
        var, eval, if
    </keywords>
</test>
*/
function main()
{
    var eval = 1
    if (eval === 1 && typeof(eval) === "number")
        return 1;
    else
        return 0;
}
