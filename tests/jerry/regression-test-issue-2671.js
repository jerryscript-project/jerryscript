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

class Shape {
  constructor (id,x,y) {
    this.id = id;
    this.x = x;
    this.y = y;
  }
  toString () {
      return "Shape(" + this.id + ")"
  }
}
class Rectangle extends Shape {
  constructor (id, x, y, width, height) {
      super (id, x, y);
  }
  toString () {
      return "Rectangle > " + super.toString ();
  }
}
class Circle extends Shape {
  constructor (id, x, y, radius) {
      super (id, x, y);
  }
  toString () {
      return "Circle > " + super.toString ();
  }
}
var shape = new Shape (0, 0, 0);
var rec = new Rectangle (1, 0, 0, 4, 4);
var circ = new Circle (2, 0, 0, 4);

assert (Object.keys (shape).toString () === "id,x,y");
assert (rec.id === 1);
assert (circ.id === 2);
assert (rec.toString () === "Rectangle > Shape(1)");
assert (circ.toString () === "Circle > Shape(2)");
