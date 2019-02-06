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

/* Create bound implicit class constructor */
class myArray extends Array { };

var array = new myArray (1);
array.push (2);
assert (array.length === 2);
assert (array instanceof myArray);
assert (array instanceof Array);
assert (!([] instanceof myArray));

/* Add a new element to the bound function chain */
class mySecretArray extends myArray { };

var secretArray = new mySecretArray (1, 2);
secretArray.push (3);
assert (secretArray.length === 3);
assert (secretArray instanceof mySecretArray);
assert (secretArray instanceof myArray);
assert (secretArray instanceof Array);
assert (!([] instanceof mySecretArray));

/* Add a new element to the bound function chain */
class myEpicSecretArray extends myArray { };

var epicSecretArray = new myEpicSecretArray (1, 2, 3);
epicSecretArray.push (4);
assert (epicSecretArray.length === 4);
assert (epicSecretArray instanceof myEpicSecretArray);
assert (epicSecretArray instanceof mySecretArray);
assert (epicSecretArray instanceof myArray);
assert (epicSecretArray instanceof Array);
assert (!([] instanceof myEpicSecretArray));
