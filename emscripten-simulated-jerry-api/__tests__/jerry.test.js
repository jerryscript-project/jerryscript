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

describe('jerry.js', () => {
  beforeEach(() => {
    Module = {
      Promise: {
        _setAsap: jest.fn(),
      },
    };
    require('../jerry');
    __jerry.reset();
  });

  it('can ref/get primitive values', () => {
    [123, null, undefined, 'abc', NaN, true, false].forEach((primitiveValue) => {
      const jerry_val = __jerry.ref(primitiveValue);

      // Should get the same value again:
      expect(__jerry.ref(primitiveValue)).toEqual(jerry_val);

      // Primitive should be in the _hostPrimitiveValueToEntryMap:
      expect(__jerry._hostPrimitiveValueToEntryMap.has(primitiveValue)).toBeTruthy();

      // get() should work:
      expect(__jerry.get(jerry_val)).toEqual(primitiveValue);
    });
  });

  it('can ref/get non-primitive values', () => {
    [{}, () =>{ }].forEach((nonPrimitiveValue) => {
      const jerry_val = __jerry.ref(nonPrimitiveValue);

      // Should get the same value again:
      expect(__jerry.ref(nonPrimitiveValue)).toEqual(jerry_val);

      // Primitive should NOT be in the _hostPrimitiveValueToEntryMap:
      expect(__jerry._hostPrimitiveValueToEntryMap.has(nonPrimitiveValue)).toBeFalsy();

      // get() should work:
      expect(__jerry.get(jerry_val)).toEqual(nonPrimitiveValue);
    });
  });
});
