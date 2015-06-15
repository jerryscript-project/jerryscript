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

assert( isNaN (Math.log(NaN)) );
assert( isNaN (Math.log (-1)) );
assert( isNaN (Math.log (-Infinity)) );

assert( Math.log (0) === -Infinity );
assert( Math.log (1) === 0 );
assert( Math.log (Infinity) === Infinity );
assert( Math.log (2) >= Math.LN2 * 0.999999 );
assert( Math.log (2) <= Math.LN2 * 1.000001 );

var very_close_to_1_but_greater = 1.0000001;
assert( very_close_to_1_but_greater > 1.0 );

assert( Math.log (very_close_to_1_but_greater) >= 0.0 );
assert( Math.log (very_close_to_1_but_greater) <= 0.000001 );

var very_close_to_1_but_less = 0.999999;
assert( very_close_to_1_but_less < 1.0 );

assert( Math.log (very_close_to_1_but_less) <= 0.0 );
assert( Math.log (very_close_to_1_but_less) >= -0.00001 );

assert( Math.log (2.7182818284590452354) >= 0.999999 );
assert( Math.log (2.7182818284590452354) <= 1.000001 );

assert( Math.log (0.000000001) <= 0.999999 * (-20.7232658369) );
assert( Math.log (0.000000001) >= 1.000001 * (-20.7232658369) );

assert( Math.log (1.0e+38) >= 0.999999 * 87.4982335338 );
assert( Math.log (1.0e+38) <= 1.000001 * 87.4982335338 );
