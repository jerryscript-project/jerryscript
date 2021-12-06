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

(function tc_25_04_05__004() {
  var a = new Promise(function(f, r){
    f("a");
  });

  var b = new Promise(function(f, r){
    f(a);
  })

  b.then(function(x) {
    assert (x === "a");
  })
})();

(function tc_25_04_05__002() {
  var obj = {name:""};

  var a = new Promise(function(f, r){
    obj.name = obj.name + "a";
    f(obj);
  });

  a.then(function(x) {
    x.name = x.name + "b";
    return x;
  }).then(null, function(x) {
    x.name = x.name + "c";
    return x;
  }).then(function(x) {
    x.name = x.name + "d";
    assert (obj.name === "aebd");
  });

  obj.name = obj.name + "e";

  assert (obj.name === "ae")
})();

(function tc_25_04_05__006() {
  var p = Promise.resolve(1).then(function(x) {
    assert(x === 1);
    return Promise.resolve(2);
  }).then(function(x) {
    assert(x === 2);
    return Promise.reject(3);
  }).catch(function(x) {
    assert(x === 3);
  });
})();

(function tc_25_04_05__001() {
  assert (Promise.length === 1);
})();

(function tc_25_04_05__003() {
  var a = new Promise(function(f, r){
    r(0);
  });

  a
  .then(function f1(x) {
    return x + 1;
  }, function r1(x){
    return x + 10;
  })
  .then(function f2(x) {
    throw x + 100
  })
  .then(function f3(x) {
    return x + 1000
  }, function r3(x) {
    return x + 10000
  })
  .then(function(x) {
    assert (x === 10110);
  })
})();

(function tc_25_04_05__005() {
  Promise.reject("abc").catch(function(x)
  {
    assert (x === "abc");
    return "def";
  }).then(function(x) {
    assert (x === "def");
  });
})();

(function tc_25_04_04__005() {
  var a = Promise.resolve('a');
  var b = Promise.resolve('b');
  var c = Promise.reject('c');

  Promise.all([a, b, 1]).then(function(x)
  {
    assert (x[0] === 'a');
    assert (x[1] === 'b');
    assert (x[2] === 1);
  }, function(x)
  {
    assert (false);
  });

  Promise.all([a, b, c, 1]).then(function(x)
  {
    assert (false);
  }, function(x)
  {
    assert (x === 'c');
  });

  Promise.all([]).then(function(x)
  {
    assert (x.length === 0);
  }, function(x)
  {
    assert (false);
  });

  Promise.all(a).then(function(x)
  {
    assert (false);
  }, function(x)
  {
    assert(x.name === "TypeError");
  });
})();

(function tc_25_04_04__001() {
  Promise.resolve("abc").then(function(x)
  {
    assert (x === "abc");
  });
})();

(function tc_25_04_04__003() {
  Promise.reject("abc").then(function(x)
  {
    assert (false);
  }, function(x)
  {
    assert (x === "abc");
  });
})();

(function tc_25_04_04__002() {
  var a = new Promise(function(f)
  {
    var o = {name: "abc"};
    f(o);
  })

  Promise.resolve(a).then(function(x)
  {
    assert (x.name === "abc");
  });
})();

(function tc_25_04_04__004() {
  var a = Promise.resolve('a');
  var b = Promise.reject('b');

  Promise.race([a, b]).then(function(x)
  {
    assert (x === 'a');
  }, function(x)
  {
    assert (false);
  });

  Promise.race([b, a]).then(function(x)
  {
    assert (false);
  }, function(x)
  {
    assert (x === 'b');
  });

  Promise.race([ ,b, a]).then(function(x)
  {
    assert (x === undefined);
  }, function(x)
  {
    assert (false);
  });

  Promise.race(a).then(function(x)
  {
    assert (false);
  }, function(x)
  {
    assert(x.name === "TypeError");
  });
})();

(function tc_25_04_03__002() {
  var name1 = "";
  var name2 = "";
  var name3 = "";
  function foo() {};

  try
  {
    new Promise();
  }
  catch (e)
  {
    name1 = e.name;
  }

  try
  {
    Promise(foo);
  }
  catch (e)
  {
    name2 = e.name;
  }

  try
  {
    new Promise("string");
  }
  catch (e)
  {
    name3 = e.name;
  }

  assert (name1 === "TypeError");
  assert (name2 === "TypeError");
  assert (name3 === "TypeError");
})();

(function tc_25_04_03__001() {
  function foo() {};

  var a = new Promise(foo);

  assert (a instanceof Promise);
})();
