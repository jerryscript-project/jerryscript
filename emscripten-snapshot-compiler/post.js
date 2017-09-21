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

var Module;
var ENVIRONMENT_IS_NODE;

(function(){
  Module.prototype.snapshotVersion = Module.ccall(
    'emscripten_snapshot_compiler_get_version', 'number');

  Module.inspect = Module.toString = function() {
    return Module.prototype.toString();
  };
})();

// Set module.exports or else expose constructor on global object:
if (typeof module !== 'undefined') {
  module.exports = Module;
} else {
  var global = (new Function('return this;'))();
  global.JerryScriptSnapshotCompiler = Module;
}

// When invoked as the main Node.js entry point, also expose a basic CLI:
if (ENVIRONMENT_IS_NODE && require.main === module) {
  function printUsageAndExit() {
    var path = require('path');
    var scriptFile = path.basename(__filename);
    console.log(
`Generates JerryScript snaphot file from JavaScript source file.

  ${scriptFile} [options] IN_FILE

OPTIONS:
  -o | --output    Snapshot output file path -- default is "snapshot.out"
  -s | --strict    Source should be interpreted in strict mode -- default is false
  -e | --eval      Code should be executed as eval() -- default is execute as global`);
    process.exit(-1);
  }

  // inputPath, isForGlobal, isStrict, outputPath
  var options = {
    inputPath: undefined,
    outputPath: 'snapshot.out',
    isForGlobal: true,
    isStrict: false,
  };

  var nextKey;
  process.argv.slice(1).forEach((val, index) => {
    switch (val) {
      case '-o':
      case '--output':
        nextKey = 'outputPath';
        break;
      case '-e':
      case '--eval':
        nextKey = '';
        options.isForGlobal = false;
        break;
      case '-s':
      case '--strict':
        options.isForGlobal = true;
        break;
      default:
        if (nextKey) {
          options[nextKey] = val;
          nextKey = undefined;
        } else {
          options.inputPath = val;
        }
    }
  });

  var fs = require('fs');
  if (!options.inputPath || !options.outputPath) {
    printUsageAndExit();
  }

  if (!fs.existsSync(options.inputPath)) {
    console.error(`Input file does not exist: ${options.inputPath}`);
    process.exit(-1);
  }

  var compiler = new Module();
  compiler.compileFile(
    options.inputPath, options.isForGlobal,
    options.isStrict, options.outputPath);
  process.exit(0);
}

})(); // IIFE that is started in pre.js