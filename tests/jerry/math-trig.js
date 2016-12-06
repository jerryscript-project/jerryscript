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

var delta = 0.0001;
var mod_m = 1.0 - delta;
var mod_p = 1.0 + delta;

assert (isNaN (Math.cos (NaN)));
assert ((Math.cos (+0.0)) == 1.0);
assert ((Math.cos (-0.0)) == 1.0);
assert (isNaN (Math.cos (Infinity)));
assert (isNaN (Math.cos (-Infinity)));

assert (Math.cos (Math.PI) > -1.0 * mod_p);
assert (Math.cos (Math.PI) < -1.0 * mod_m);

assert (Math.cos (Math.PI / 2) > -delta);
assert (Math.cos (Math.PI / 2) < +delta);
assert (Math.cos (-Math.PI / 2) > -delta);
assert (Math.cos (-Math.PI / 2) < +delta);

assert (Math.cos (Math.PI / 4) > mod_m * Math.SQRT2 / 2);
assert (Math.cos (Math.PI / 4) < mod_p * Math.SQRT2 / 2);

assert (Math.cos (-Math.PI / 4) > mod_m * Math.SQRT2 / 2);
assert (Math.cos (-Math.PI / 4) < mod_p * Math.SQRT2 / 2);

assert (isNaN (Math.sin (NaN)));
assert (1.0 / Math.sin (0.0) == Infinity);
assert (1.0 / Math.sin (-0.0) == -Infinity);
assert (isNaN (Math.sin (Infinity)));
assert (isNaN (Math.sin (-Infinity)));

assert (Math.sin (Math.PI) > -delta);
assert (Math.sin (Math.PI) < +delta);

assert (Math.sin (Math.PI / 2) > 1.0 * mod_m);
assert (Math.sin (Math.PI / 2) < 1.0 * mod_p);

assert (Math.sin (-Math.PI / 2) > -1.0 * mod_p);
assert (Math.sin (-Math.PI / 2) < -1.0 * mod_m);

assert (Math.sin (Math.PI / 4) > mod_m * Math.SQRT2 / 2);
assert (Math.sin (Math.PI / 4) < mod_p * Math.SQRT2 / 2);

assert (Math.sin (-Math.PI / 4) > -mod_p * Math.SQRT2 / 2);
assert (Math.sin (-Math.PI / 4) < -mod_m * Math.SQRT2 / 2);

var step = 0.01;

for (var x = -2 * Math.PI; x <= 2 * Math.PI; x += step)
{
  var s = Math.sin (x);
  var c = Math.cos (x);
  var sqr_s = s * s;
  var sqr_c = c * c;

  assert (sqr_s + sqr_c > mod_m);
  assert (sqr_s + sqr_c < mod_p);
}
