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

import { Breakpoint, ParsedFunction } from '../breakpoint';

describe('Breakpoint constructor', () => {
  const mockParsedFunction: any = {
    scriptId: 42,
  };

  it('assigns values from options arg', () => {
    const bp = new Breakpoint({
      scriptId: 1,
      func: mockParsedFunction,
      line: 42,
      offset: 37,
      activeIndex: 5,
    });
    expect(bp.scriptId).toEqual(1);
    expect(bp.func).toEqual(mockParsedFunction);
    expect(bp.line).toEqual(42);
    expect(bp.offset).toEqual(37);
    expect(bp.activeIndex).toEqual(5);
  });

  it('sets activeIndex to -1 by default', () => {
    const bp = new Breakpoint({
      scriptId: 1,
      func: mockParsedFunction,
      line: 42,
      offset: 37,
    });
    expect(bp.scriptId).toEqual(1);
    expect(bp.func).toEqual(mockParsedFunction);
    expect(bp.line).toEqual(42);
    expect(bp.offset).toEqual(37);
    expect(bp.activeIndex).toEqual(-1);
  });
});

describe('Breakpoint toString', () => {
  const mockParsedFunction: any = {
    line: 21,
    column: 61,
  };

  beforeEach(() => {
    mockParsedFunction.sourceName = 'jerry.js',
    mockParsedFunction.isFunc = true;
    mockParsedFunction.name = undefined;
  });

  it('displays function name, line, and column', () => {
    mockParsedFunction.name = 'cheese';

    const bp = new Breakpoint({
      scriptId: 1,
      func: mockParsedFunction,
      line: 42,
      offset: 37,
    });
    expect(bp.toString()).toEqual('jerry.js:42 (in cheese() at line:21, col:61)');
  });

  it('shows "function" for unnamed functions', () => {
    const bp = new Breakpoint({
      scriptId: 1,
      func: mockParsedFunction,
      line: 42,
      offset: 37,
    });
    expect(bp.toString()).toEqual('jerry.js:42 (in function() at line:21, col:61)');
  });

  it('drops function detail if not really a function (i.e. global scope)', () => {
    mockParsedFunction.isFunc = false;

    const bp = new Breakpoint({
      scriptId: 1,
      func: mockParsedFunction,
      line: 42,
      offset: 37,
    });
    expect(bp.toString()).toEqual('jerry.js:42');
  });

  it('reports source name as "<unknown>" if not given', () => {
    mockParsedFunction.isFunc = false;
    mockParsedFunction.sourceName = undefined;

    const bp = new Breakpoint({
      scriptId: 1,
      func: mockParsedFunction,
      line: 42,
      offset: 37,
    });
    expect(bp.toString()).toEqual('<unknown>:42');
  });
});

describe('ParsedFunction constructor', () => {
  it('adds a breakpoint for each line/offset pair in the frame', () => {
    const frame = {
      isFunc: true,
      scriptId: 42,
      line: 1,
      column: 2,
      source: '',
      sourceName: 'cheese.js',
      name: 'cheddar',
      lines: [4, 9, 16, 25],
      offsets: [8, 27, 64, 125],
    };

    const pf = new ParsedFunction(7, frame);
    expect(pf.isFunc).toEqual(true);
    expect(pf.byteCodeCP).toEqual(7);
    expect(pf.scriptId).toEqual(42);
    expect(pf.line).toEqual(1);
    expect(pf.column).toEqual(2);
    expect(pf.name).toEqual('cheddar');
    expect(pf.firstBreakpointLine).toEqual(4);
    expect(pf.firstBreakpointOffset).toEqual(8);
    expect(pf.sourceName).toEqual('cheese.js');

    // the third breakpoint w/ line 16, offset 64 is indexed as 16 in lines
    expect(pf.lines[16].line).toEqual(16);
    expect(pf.lines[16].offset).toEqual(64);

    // the fourth breakpoint w/ line 25, offset 125 is indexed as 125 in offsets
    expect(pf.offsets[125].line).toEqual(25);
    expect(pf.offsets[125].offset).toEqual(125);
  });
});
