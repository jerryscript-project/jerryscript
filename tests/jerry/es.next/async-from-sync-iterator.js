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

async function f() {
    let arr_idx = 0;
    for await (let a of [0, 1, 2, 3]) {
        assert(arr_idx++ == a);
    }

    let char_code = "a".charCodeAt(0);
    for await (let a of "abc") {
        assert(char_code++ == a.charCodeAt(0));
    }

    let set_idx = 0;
    for await (let a of new Set([0, 1, 2, 3])) {
        assert(set_idx++ == a);
    }

    let map_idx = 0;
    for await (let [key, value] of new Map([0, 1, 2, 3].entries())) {
        assert(map_idx++ == value);
    }
}

async function* asyncg(obj) {
    yield* obj;
}

async function f1() {
    var caught = false;
    var iter = asyncg({
        get [Symbol.iterator]() {
            throw "Symbol.iteratorError"
        },
    });

    iter.next().catch(e => {
        caught = true;
        assert(e === "Symbol.iteratorError")
    }).then(e => {
        assert(caught)
    });
}

async function f2() {
    var caught = false;
    var iter = asyncg({
        [Symbol.iterator]() {
            return {
                next() {
                    throw "nextError";
                },
            };
        },
    });

    iter.next().catch(e => {
        caught = true;
        assert(e === "nextError")
    }).then(e => {
        assert(caught)
    });
}

async function f3() {
    var caught = false;
    var iter = asyncg({
        [Symbol.iterator]() {
            return {
                next() {
                    return {
                        get value() {
                            throw "valueError"
                        },
                        done: false
                    };
                },
            };
        },
    });

    iter.next().catch(e => {
        caught = true;
        assert(e === "valueError")
    }).then(e => {
        assert(caught)
    });
}

async function f4() {
    var caught = false;
    var iter = asyncg({
        [Symbol.iterator]() {
            return {
                next() {
                    return {
                        value: "value",
                        get done() {
                            throw "doneError"
                        },
                    };
                },
            };
        },
    });

    iter.next().catch(e => {
        caught = true;
        assert(e === "doneError")
    }).then(e => {
        assert(caught)
    });
}

async function f5() {
    var caught = false;

    var iter = asyncg({
        [Symbol.iterator]() {
            return {
                next() {
                    return {
                        value: 1,
                        done: false
                    };
                },
                get return () {
                    throw "returnError"
                }
            };
        }
    });

    iter.next().then(function (res) {
        assert(res.value === 1);
        assert(!res.done);
        iter.return().catch(e => {
            caught = true;
            assert(e == "returnError");
        }).then(e => {
            assert(caught);
        });
    });
}

async function f6() {
    var caught = false;

    var iter = asyncg({
        [Symbol.iterator]() {
            return {
                next() {
                    return {
                        value: 1,
                        done: false
                    };
                },
                get throw () {
                    throw "throwError"
                }
            };
        }
    });

    iter.next().then(function (res) {
        assert(res.value === 1);
        assert(!res.done);
        iter.throw().catch(e => {
            caught = true;
            assert(e == "throwError");
        }).then(e => {
            assert(caught);
        });
    });
}

const tests = [f, f1, f2, f3, f4, f5, f6];

for (let t of tests) {
    t();
}

