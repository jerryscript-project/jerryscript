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

// tokenize
0.0 0. .0 0e0 0e+0 0e-0 0.0e0 0.0e+0 0.0e-0 .0e0 .0e+0 .0e-0
1.0 1. .1 1E0 1E+0 1E-0 1.0E0 1.0E+0 1.0E-0 .1e0 .1e+0 .1e-0
123.456 123. .456 123e78 123E+78 123e-78 123.456E78 123.456E+78 123.456E-78 .456e78 .456E+78 .456E-78
123e18 123E+18 123e-18 123.456E18 123.456E+18 123.456E-18 .456e18 .456E+18 .456E-18