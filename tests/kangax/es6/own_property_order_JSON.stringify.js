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

        var obj = {
          2: true,
          0: true,
          1: true,
          ' ': true,
          9: true,
          D: true,
          B: true,
          '-1': true
        };
        obj.A = true;
        obj[3] = true;
        "EFGHIJKLMNOPQRSTUVWXYZ".split('').forEach(function(key){
          obj[key] = true;
        });
        Object.defineProperty(obj, 'C', { value: true, enumerable: true });
        Object.defineProperty(obj, '4', { value: true, enumerable: true });
        delete obj[2];
        obj[2] = true;

        return JSON.stringify(obj) ===
          '{"0":true,"1":true,"2":true,"3":true,"4":true,"9":true," ":true,"D":true,"B":true,"-1":true,"A":true,"E":true,"F":true,"G":true,"H":true,"I":true,"J":true,"K":true,"L":true,"M":true,"N":true,"O":true,"P":true,"Q":true,"R":true,"S":true,"T":true,"U":true,"V":true,"W":true,"X":true,"Y":true,"Z":true,"C":true}';
      
}

if (!test())
    throw new Error("Test failed");
