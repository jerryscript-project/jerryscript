// Copyright 2016 Samsung Electronics Co., Ltd.
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

var counter = 0;

({ get "0"() { counter += 1; } })[0];
({ get 0() { counter += 2; } })[0];
({ get 0.0() { counter += 3; } })[0];
({ get 0.() { counter += 4; } })[0];
({ get 1.() { counter += 5; } })[1];
({ get 5.2322341234123() { counter += 6; } })[5.2322341234123];

assert (counter == 21);

({ set "0"(q) { counter -= 1; } })[0] = "dummy";
({ set 0(q) { counter -= 2; } })[0] = "dummy";
({ set 0.0(q) { counter -= 3; } })[0] = "dummy";
({ set 0.(q) { counter -= 4; } })[0] = "dummy";
({ set 1.(q) { counter -= 5; } })[1] = "dummy";
({ set 5.2322341234123(q) { counter -= 6; } })[5.2322341234123] = "dummy";

assert (counter == 0);
