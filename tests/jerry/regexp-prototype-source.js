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

var regexp = /fooBar/ig;
assert(regexp.source === 'fooBar');

assert(new RegExp().source === '(?:)');

assert(new RegExp('/foo/').source === '\\/foo\\/');
assert(new RegExp('/foo/').source.length === 7);

assert(new RegExp('bar', 'ug').source === 'bar');

assert(new RegExp('/\?/').source === '\\/?\\/');
assert(new RegExp('/\?/').source.length === 5);

assert(new RegExp('\n').source === '\\n');

assert(new RegExp('\r').source === '\\r');

assert(new RegExp('\u2028').source === '\\u2028');

assert(new RegExp('\u2029').source === '\\u2029');

assert(new RegExp('/\n/').source === '\\/\\n\\/');
assert(new RegExp('/\n/').source.length === 6);

assert(new RegExp(/\/\//).source === '\\/\\/');
assert(new RegExp(/\?\//g).source === '\\?\\/');

assert (RegExp.prototype.source === '(?:)')

var sourceProp = Object.getOwnPropertyDescriptor (RegExp.prototype, "source");
try {
  sourceProp.get.call({});
  assert (false);
} catch (e) {
  assert (e instanceof TypeError);
}
