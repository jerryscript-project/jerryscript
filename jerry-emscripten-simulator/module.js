/* Copyright 2016 Pebble Technology Corp.
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

var Module = {
  preRun: function() {
    if (typeof(process) === 'undefined') {
      throw new Error('Make sure to run this code in Node.js');
    }
    var nodeStdinCallback = process.stdin.read.bind(process.stdin);
    var stdin = function() {
      console.log('stdin called');
      return null;
    };
    FS.init(stdin);
  }
};
