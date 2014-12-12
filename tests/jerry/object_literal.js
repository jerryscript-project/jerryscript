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

var person = {
    firstName:"John",
    lastName:"Doe",
    age:50,
    eyeColor:"blue",
    "gender":"male",
    0:0
};

assert (person.firstName === "John");
assert (person["firstName"] === "John");
assert (person.lastName === "Doe");
assert (person["lastName"] === "Doe");
assert (person.age === 50);
assert (person["age"] === 50);
assert (person.eyeColor === "blue");
assert (person["eyeColor"] === "blue");
assert (person.gender === "male");
assert (person["gender"] === "male");
assert (person["0"] === 0);

var x = person;
x.age = 40;
assert (x.age === 40);
assert (person.age === 40);

var john = new Object();
john.firstName = "John";
john.lastName = "Doe";
john.age = 40;
john.eyeColor = "blue";

assert (person.firstName === john.firstName);
assert (person.lastName === john.lastName);
assert (person.age === john.age);
assert (person.eyeColor === john.eyeColor);

var a, b; 
b = 'property1'; 
a = { 
  'property1' : 'value1', 
  get property2 () { return 1; }, 
  set property2 (a) {
    if (true)
      this.property3 = a * 10;
    else
      this.property3 = a;
  },
  set property3 (b) { this.property1 = b; }
}; 
assert (a.property1 === 'value1'); 
assert (a.property2 === 1); 
a.property3 = 'value2'; 
assert (a.property1 === 'value2'); 
a.property2 = 2.5; 
assert (a.property1 === 25); 
b = delete a[b]; 
assert (b === true); 
assert (a.property1 === undefined); 
