// Copyright 2015 Samsung Electronics Co., Ltd.
// Copyright 2015 University of Szeged.
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

try {
    Boolean(decodeURI(decodeURIComponent(Number())));
} catch(err) {}
 try {
    ReferenceError(isNaN(__proto__));
 } catch(err) {}
 try {
    isNaN(__proto__);
 } catch(err) {}
 try {
    load();
 } catch(err) {}
 try {
    RegExp("\n");
 } catch(err) {}
 try {} catch(err) {}
 try {
    ReferenceError(performance.__proto__.isPrototypeOf(arguments.map(os)));
 } catch(err) {}
 try {
    Float64Array(performance,WeakSet(),Infinity.valueOf());
 } catch(err) {}
 try {
    arguments.push(Int8Array(Boolean(isFinite(quit())),ArrayBuffer(os.system()),Array(read())));
 } catch(err) {}
 try {
    Boolean(encodeURI(DataView(ArrayBuffer(os),parseFloat(Set()),URIError(Object(Int8Array(Function(parseInt(write(),RangeError(__proto__.valueOf()))),Int16Array(Map(),__proto__.valueOf(),readbuffer()),Math))))));
 } catch(err) {}
