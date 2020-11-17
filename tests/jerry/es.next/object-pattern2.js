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

function checkSyntax (str) {
  try {
    eval (str);
    assert (false);
  } catch (e) {
    assert (e instanceof SyntaxError);
  }
}

checkSyntax ("var {a, ...b, a} = {}")
checkSyntax ("var {a, ...b = 6} = {}")
checkSyntax ("var {a, ...[b]} = {}")
checkSyntax ("function f({a, ...a}) {}")
checkSyntax ("let {a, ...a} = {}")
checkSyntax ("let {a, ...(b)} = {}")
checkSyntax ("({a, ...{b}} = {})")
checkSyntax ("({a, ...([b])} = {})")
checkSyntax ("({a, ...b().a = 6} = {})")
checkSyntax ("try {} catch ({ ...a } = {}) {}")
checkSyntax ("for ({ ...a } = {} of []) {}")

var sym1 = Symbol("sym1")
var sym2 = Symbol("sym2")

function rest_compare(rest, values)
{
  var keys = Reflect.ownKeys(rest);

  assert(keys.length * 2 === values.length)

  for (var i = 0; i < keys.length; i++) {
    key = keys[i]
    assert(key === values[i * 2])
    assert(rest[key] === values[i * 2 + 1])
  }
}

function f1() {
  var { bb, ...rest } = { a:true, bb:6.25, [sym1]:"X" }

  assert(bb === 6.25)
  rest_compare(rest, ["a", true, sym1, "X"])
}
f1()

function f2() {
  var a, b, rest = {};

  ({ a, "+": b, ...rest.c } = { "+": -3.75, [sym1]:6.25, [sym2]: sym2 })

  assert(a === undefined)
  assert(b === -3.75)
  rest_compare(rest.c, [sym1, 6.25, sym2, sym2])
}
f2()

function f3() {
  var a, rest1, rest2;

  ({ A:{ [sym2]: a, ...rest1 }, B:{ ...rest2 } = { X:null, 7:"A" } } = { A:{ [sym1]: 7.5, [sym2]: 3.5, S:-5.5 } })

  assert(a === 3.5)
  rest_compare(rest1, ["S", -5.5, sym1, 7.5])
  rest_compare(rest2, ["7", "A", "X", null])
}
f3()

function f4() {
  try {
    throw { A:5, [sym1]:6, [sym2]:7, b:8 }
    assert(false)
  } catch({ b, c, ...rest }) {
    assert(b === 8)
    assert(c === undefined)
    rest_compare(rest, ["A", 5, sym1, 6, sym2, 7])
  }
}
f4()

function f5() {
  var a = {}, b = {}, c = {}, d = () => c

  for ({ A:a.x, B:b.y, ...d().z } of [ { B:"AA", C:6.25, [sym1]:-4.5 } ]) {
    assert(a.x === undefined)
    assert(typeof Object.getOwnPropertyDescriptor(a, "x") === "object")
    assert(b.y === "AA")
    rest_compare(c.z, ["C", 6.25, sym1, -4.5])
  }
}
f5()

function f6({ [(() => "A")() + "A"]:a, ...b },
            { A:c, [sym1]:d, [sym2]:e, ...f } = { A:"X", [sym2]: 5.5 }) {
  assert(a === 6)
  rest_compare(b, ["B", 7, sym1, 8.25])
  assert(c === "X")
  assert(d === undefined)
  assert(e === 5.5)
  rest_compare(f, [])
}
f6({ [sym1]:8.25, B:7, AA:6 })

function f7() {
  var b, o = {}, rest

  ({ ["A" + "A"]:b, ...{o}.o.rest } = { [sym1]:5.5, A:"X", AA:-0.25 })

  assert(b === -0.25)
  rest_compare(o.rest, [ "A", "X", sym1, 5.5])
}
f7()

function f8() {
  var a, b;

  ({ ...(b) } = { })
  rest_compare(b, []);

  [a,a,...{ ...b  }] = ["A", "B", "C", "D"]
  rest_compare(b, ["0", "C", "1", "D"])
}
f8()

function f9() {
  var counter = 0;

  for (const { ...rest} in { B: "AA", C: 6.25 }) {
    switch (counter) {
      case 0: {
        /* rest === { '0': 'B' } */
        assert(rest['0'] === 'B');
        break;
        }
      case 1: {
        /* rest === { '0': 'C' } */
        assert(rest['0'] == 'C');
        break;
      }
    }
    counter++;
  }
  assert(counter === 2);
}
f9()
