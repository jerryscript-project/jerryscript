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

// helper function - simple implementation
Array.prototype.equals = function (array) {
  if (this.length != array.length)
    return false;

  for (var i = 0; i < this.length; i++) {
    if (this[i] instanceof Array && array[i] instanceof Array) {
      if (!this[i].equals(array[i]))
        return false;
      }
      else if (this[i] != array[i]) {
        return false;
    }
  }

  return true;
}

function longDenseArray(){
        var a = [0];
        for(var i = 0; i < 60; i++){
                a[i] = i;
        }
        return a;
}

function shorten(){
        currArray.length = 20;
        return 1;
}

var array = [0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,,,,,,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19];
var currArray = longDenseArray();
currArray.copyWithin(25, {valueOf: shorten})
assert (currArray.length == 44)
assert (currArray.equals (array))
