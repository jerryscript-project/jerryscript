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

import child from 'child_process';
import fs from 'fs';
import path from 'path';
import test from 'ava';

const compilerFile = path.join(
  __dirname, '..', '..','build/bin/jerryscript-snapshot-compiler.js');

function fixture(name) {
  return path.join(__dirname, 'fixtures', name);
}

function invokeCli() {
  const args = new Array(compilerFile, ...arguments);
  child.spawnSync(process.execPath, args, {
    stdio: 'inherit'
  });
}

function testHelper(t, outPath) {
  t.false(fs.existsSync(outPath));
  const args = new Array(...arguments, fixture('sample.js'));
  invokeCli.apply(null, args);
  t.true(fs.existsSync(outPath));
  fs.unlinkSync(outPath);  
}

test('Input file only', t => {
  const outPath = 'snapshot.out'
  testHelper(t, outPath);
});

test('-o | --output', t => {
  ['-o', '--output'].forEach(option => {
    const tmpDir = fs.mkdtempSync('temp-jrs');
    const outPath = path.join(tmpDir, 'test.snapshot');  
    testHelper(t, outPath, option, outPath);
    fs.rmdirSync(tmpDir);
  });
});

test('-s | --strict', t => {
  ['-s', '--strict'].forEach(option => {
    const outPath = 'snapshot.out'
    testHelper(t, outPath, option);
  });
});

test('-e | --eval', t => {
  ['-e', '--eval'].forEach(option => {
    const outPath = 'snapshot.out'
    testHelper(t, outPath, option);
  });
});
