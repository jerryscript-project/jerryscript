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

for (var y = 1950; y < 2050; y++) {
  for (var m = 0; m < 12; m++) {
    var last_date = new Date(y, m, 1).getDay ();
    assert (!isNaN (last_date));
    for (var d = 1; d < 32; d++) {
      assert (last_date == new Date(y, m, d).getDay ());
      last_date = (last_date + 1) % 7;
    }
  }
}
