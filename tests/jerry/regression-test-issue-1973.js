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

print (777E7777777777 == Infinity)
print (-777E7777777777 == -Infinity)
print (777E-7777777777 == 0)
print (-777E-7777777777 == -0)

print (100E307 == Infinity)
print (10E307 == 1E308)
print (10E308 == Infinity)
print (1E308 == 1E308)
print (0.1E309 == 1E308)
print (0.1E310 == Infinity)

print (-100E307 == -Infinity)
print (-10E307 == -1E308)
print (-10E308 == -Infinity)
print (-1E308 == -1E308)
print (-0.1E309 == -1E308)
print (-0.1E310 == -Infinity)

print (5E-325 == 0)
print (50E-325 == 5E-324)
print (0.5E-324 == 0)
print (5E-324 == 5E-324)
print (0.05E-323 == 0)
print (0.5E-323 == 5E-324)

print (-5E-325 == -0)
print (-50E-325 == -5E-324)
print (-0.5E-324 == -0)
print (-5E-324 == -5E-324)
print (-0.05E-323 == -0)
print (-0.5E-323 == -5E-324)
