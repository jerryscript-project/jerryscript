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

class Animal {
  constructor (name) {
    this.name = name;
  }

  hello () {
    return "Hello I am " + this.name;
  }

  static speak () {
    return "Animals roar.";
  }

  static explain () {
    return "I can walk,";
  }

  whoAmI () {
    return "I am an Animal.";
  }

  breath () {
    return "I am breathing.";
  }

  get myName () {
    return this.name;
  }

  set rename (name) {
    this.name = name;
  }
}

class Dog extends Animal {
  constructor (name, barks) {
    super (name);
    this.barks = barks;
  }

  hello () {
    return super.hello () + " and I can " + (this.barks ? "bark" : "not bark");
  }

  whoAmI () {
    return "I am a Dog.";
  }

  static speak () {
    return "Dogs bark.";
  }

  static explain () {
    return super.explain () + " jump,";
  }

  bark () {
    return this.barks ? "Woof" : "----";
  }
}

class Doge extends Dog {
  constructor (name, barks, awesomeness) {
    super (name, barks);
    this.awesomeness = awesomeness;
  }

  hello () {
    return super.hello () + " and I'm " + (this.awesomeness > 9000 ? "super awesome" : "awesome") + ".";
  }

  whoAmI ( ) {
    return "I am a Doge.";
  }

  static speak () {
    return "Doges wow.";
  }

  static explain () {
    return super.explain () + " dance.";
  }
}

var doge = new Doge ("doggoe", true, 10000);
assert (doge.name === "doggoe");
doge.rename = "doggo";
assert (doge.myName === "doggo");
assert (doge.barks === true);
assert (doge.awesomeness === 10000);
assert (doge.hello () === "Hello I am doggo and I can bark and I'm super awesome.");
assert (doge.whoAmI () === "I am a Doge.");
assert (doge.breath () === "I am breathing.");
assert (doge.bark () === "Woof");
assert (Doge.speak () === "Doges wow.");
assert (Doge.explain () === "I can walk, jump, dance.");
assert (doge instanceof Animal);
assert (doge instanceof Dog);
assert (doge instanceof Doge);
assert (Dog.prototype.constructor === Dog)
assert (Doge.prototype.constructor === Doge)
