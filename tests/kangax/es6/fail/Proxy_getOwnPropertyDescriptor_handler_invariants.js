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
 *
 * This file is based on work under the following copyright and permission
 * notice:
 *
 *     Copyright (c) 2010-2013 Juriy Zaytsev
 *
 *     Permission is hereby granted, free of charge, to any person obtaining a copy
 *     of this software and associated documentation files (the "Software"), to deal
 *     in the Software without restriction, including without limitation the rights
 *     to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *     copies of the Software, and to permit persons to whom the Software is
 *     furnished to do so, subject to the following conditions:
 *
 *     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *     IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *     FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *     AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *     LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *     OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *     SOFTWARE.
 */

function test() {

        var passed = false;
        new Proxy({},{});
        // A property cannot be reported as non-existent, if it exists as a non-configurable
        // own property of the target object.
        var proxied = {};
        var proxy = new Proxy(proxied, {
          getOwnPropertyDescriptor: function () {
            passed = true;
            return undefined;
          }
        });
        Object.defineProperty(proxied, "foo", { value: 2, writable: true, enumerable: true });
        try {
          Object.getOwnPropertyDescriptor(proxy, "foo");
          return false;
        } catch(e) {}
        // A property cannot be reported as non-existent, if it exists as an own property
        // of the target object and the target object is not extensible.
        proxied.bar = 3;
        Object.preventExtensions(proxied);
        try {
          Object.getOwnPropertyDescriptor(proxy, "bar");
          return false;
        } catch(e) {}
        // A property cannot be reported as existent, if it does not exists as an own property
        // of the target object and the target object is not extensible.
        try {
          Object.getOwnPropertyDescriptor(new Proxy(proxied, {
            getOwnPropertyDescriptor: function() {
              return { value: 2, configurable: true, writable: true, enumerable: true };
            }}), "baz");
          return false;
        } catch(e) {}
        // A property cannot be reported as non-configurable, if it does not exists as an own
        // property of the target object or if it exists as a configurable own property of
        // the target object.
        try {
          Object.getOwnPropertyDescriptor(new Proxy({}, {
            getOwnPropertyDescriptor: function() {
              return { value: 2, configurable: false, writable: true, enumerable: true };
            }}), "baz");
          return false;
        } catch(e) {}
        try {
          Object.getOwnPropertyDescriptor(new Proxy({baz:1}, {
            getOwnPropertyDescriptor: function() {
              return { value: 1, configurable: false, writable: true, enumerable: true };
            }}), "baz");
          return false;
        } catch(e) {}
        return passed;
      
}

if (!test())
    throw new Error("Test failed");
