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
<test file="12.02.01-003.js">
    <author>s.ryabkova</author>
    <checks>
        Check that in non strict mode variable with name "arguments"
	could be created
    </checks>
    <keywords>
        var, arguments, if
    </keywords>
</test>
*/
function main()
{
    var arguments=2;
    if (arguments === 2 && typeof(arguments) === "number")
        return 1;
    else
        return 0;
}
