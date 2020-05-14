/* Copyright JS Foundation and other contributors, http://js.foundation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

try {
 Reflect.construct (Date,);
 assert (false);
} catch (e) {
 assert (e instanceof TypeError);
}

try {
 Reflect.construct (Date, "2015-01-01");
 assert (false);
} catch (e) {
 assert (e instanceof TypeError);
}

try {
 Reflect.construct (Date, 1420070400000);
 assert (false);
} catch (e) {
 assert (e instanceof TypeError);
}

class MyDate extends Date {};
var d1= new MyDate();

assert(Object.getPrototypeOf(d1) == MyDate.prototype)
