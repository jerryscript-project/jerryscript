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

import { ParserStackFrame } from './protocol-handler';

export interface BreakpointMap {
  [index: number]: Breakpoint;
}

export interface BreakpointOptions {
  scriptId: number;
  func: ParsedFunction;
  line: number;
  offset: number;
  activeIndex?: number;
}

export class Breakpoint {
  readonly scriptId: number;
  readonly func: ParsedFunction;
  readonly line: number;
  readonly offset: number;
  activeIndex: number = -1;

  constructor(options: BreakpointOptions) {
    this.scriptId = options.scriptId;
    this.func = options.func;
    this.line = options.line;
    this.offset = options.offset;
    if (options.activeIndex !== undefined) {
      this.activeIndex = options.activeIndex;
    }
  }

  toString() {
    const sourceName = this.func.sourceName || '<unknown>';

    let detail = '';
    if (this.func.isFunc) {
      detail = ` (in ${this.func.name || 'function'}() at line:${this.func.line}, col:${this.func.column})`;
    }

    return `${sourceName}:${this.line}${detail}`;
  }
}

export class ParsedFunction {
  readonly isFunc: boolean;
  readonly scriptId: number;
  readonly byteCodeCP: number;
  readonly line: number;
  readonly column: number;
  readonly name: string;
  readonly firstBreakpointLine: number;
  readonly firstBreakpointOffset: number;
  readonly lines: BreakpointMap = {};
  readonly offsets: BreakpointMap = {};
  source?: Array<string>;
  sourceName?: string;

  constructor(byteCodeCP: number, frame: ParserStackFrame) {
    this.isFunc = frame.isFunc;
    this.byteCodeCP = byteCodeCP;
    this.scriptId = frame.scriptId;
    this.line = frame.line;
    this.column = frame.column;
    this.name = frame.name;
    this.firstBreakpointLine = frame.lines[0];
    this.firstBreakpointOffset = frame.offsets[0];
    this.sourceName = frame.sourceName;

    for (let i = 0; i < frame.lines.length; i++) {
      const breakpoint = new Breakpoint({
        scriptId: frame.scriptId,
        func: this,
        line: frame.lines[i],
        offset: frame.offsets[i],
        activeIndex: -1,
      });

      this.lines[breakpoint.line] = breakpoint;
      this.offsets[breakpoint.offset] = breakpoint;
    }
  }
}
