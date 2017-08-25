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

import path from 'path';
import test from 'ava';

const Compiler = require('../../build/bin/jerryscript-snapshot-compiler.js');

function fixture(name) {
  return path.join(__dirname, 'fixtures', name);
}

test('Compiler is a function', t => {
  t.is(typeof Compiler, 'function');
});

test('Compiler instance has a compileString() method', t => {
  const c = new Compiler();
  t.is(typeof c.compileString, 'function');
});

test('Compiler instance has a compileFile() method', t => {
  const c = new Compiler();
  t.is(typeof c.compileFile, 'function');
});

test('Compiler.toString()', t => {
  const c = new Compiler();
  t.is(String(Compiler),
       `[JerryScriptSnapshotCompiler v${c.snapshotVersion}]`);
});

test('compiler.compileFile() method', t => {
  const c = new Compiler();
  t.notThrows(() => {
    const snapshot = c.compileFile(fixture('sample.js'));
    t.true(snapshot instanceof Uint8Array);
  });
});

test('compiler.compileFile() throws upon non-UTF8 file', t => {
  const c = new Compiler();
  t.throws(() => {
    c.compileFile(fixture('non-utf8.js'));
  }, 'Input must be valid UTF-8');
});

test('compiler.compileString()', t => {
  const c = new Compiler();
  t.notThrows(() => {
    const snapshot = c.compileString('1+1');
    t.true(snapshot instanceof Uint8Array);
  });
});

test('compiler.compileString() empty source string', t => {
  const c = new Compiler();
  t.notThrows(() => {
    const snapshot = c.compileString('');
    t.true(snapshot instanceof Uint8Array);
  });
});

test('compiler.compileString() throws upon parse error', t => {
  const c = new Compiler();
  t.throws(() => {
    c.compileString('this should trip a syntax error :-P');
  }, 'SyntaxError: Expected \';\' token. [line: 1, column: 6]');
});

test('compiler.compileFile() can be invoked multiple times', t => {
  const c = new Compiler();
  t.notThrows(() => {
    for (let i = 0; i < 10; ++i) {
      const snapshot = c.compileFile(fixture('sample.js'));
      t.true(snapshot instanceof Uint8Array);
    }
  });
});
