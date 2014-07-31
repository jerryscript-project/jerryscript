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

function assertTrue (what) {
	if (what !== true)
		exit (1);
}

function assertFalse (what) {
	if (what !== false)
		exit (1);
}

function assertNull (what) {
	if (what !== null)
		exit (1);
}

function assertNotNull (what) {
	if (what === null)
		exit (1);
}

function assertEquals (arg1, arg2) {
	if (arg1 !== arg2)
		exit (1);
}

function assertUnreachable () {
	exit (1);
}
