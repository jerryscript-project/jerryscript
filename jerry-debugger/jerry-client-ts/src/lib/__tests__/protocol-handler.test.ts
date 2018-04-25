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

import * as SP from '../jrs-protocol-constants';
import { Breakpoint } from '../breakpoint';
import { JerryDebugProtocolHandler } from '../protocol-handler';

// utility function
function encodeArray(byte: number, str: string) {
  const array = new Uint8Array(1 + str.length);
  array[0] = byte & 0xff;
  for (let i = 0; i < str.length; i++) {
    array[i + 1] = str.charCodeAt(i);
  }
  return array;
}

function setupHaltedProtocolHandler() {
  const debugClient = {
    send: jest.fn(),
  };
  const handler = new JerryDebugProtocolHandler({});
  handler.debuggerClient = debugClient as any;
  // For these tests mock the current breakpoint by setting the private lastBreakpointHit member:
  (handler as any).lastBreakpointHit = {} as Breakpoint;
  return { handler, debugClient };
}

describe('onConfiguration', () => {
  const delegate = {
    onError: jest.fn(),
  };
  const handler = new JerryDebugProtocolHandler(delegate);

  beforeEach(() => {
    delegate.onError.mockClear();
  });

  it('aborts when message too short', () => {
    const array = Uint8Array.from([1, 2, 3, 4]);
    handler.onConfiguration(array);
    expect(delegate.onError).toHaveBeenCalledTimes(1);
  });

  it('allows otherwise valid message to be too long', () => {
    const array = Uint8Array.from([0, 200, 4, 1, 2, 0]);
    handler.onConfiguration(array);
    expect(delegate.onError).toHaveBeenCalledTimes(0);
  });

  it('aborts when compressed pointer wrong size', () => {
    const array = Uint8Array.from([0, 200, 6, 1, 2]);
    handler.onConfiguration(array);
    expect(delegate.onError).toHaveBeenCalledTimes(1);
  });

  it('aborts when version unexpected', () => {
    const array = Uint8Array.from([0, 200, 4, 1, 3]);
    handler.onConfiguration(array);
    expect(delegate.onError).toHaveBeenCalledTimes(1);
  });

  it('succeeds when everything is normal', () => {
    const array = Uint8Array.from([0, 200, 4, 1, 2]);
    handler.onConfiguration(array);
    expect(delegate.onError).toHaveBeenCalledTimes(0);
  });
});

describe('onByteCodeCP', () => {
  const delegate = {
    onScriptParsed: jest.fn(),
  };
  let handler: JerryDebugProtocolHandler;

  beforeEach(() => {
    delegate.onScriptParsed.mockClear();
    handler = new JerryDebugProtocolHandler(delegate);
  });

  it('throws if stack empty', () => {
    const array = Uint8Array.from([SP.JERRY_DEBUGGER_BYTE_CODE_CP]);
    expect(() => handler.onByteCodeCP(array)).toThrow();
  });
});

describe('onSourceCode', () => {
  const delegate = {
    onScriptParsed: jest.fn(),
  };
  let handler: JerryDebugProtocolHandler;

  beforeEach(() => {
    delegate.onScriptParsed.mockClear();
    handler = new JerryDebugProtocolHandler(delegate);
  });

  it('does not call scriptParsed after only SOURCE message', () => {
    const array = encodeArray(SP.JERRY_DEBUGGER_SOURCE_CODE, 'abc');
    // code = 'abc'
    handler.onSourceCode(array);
    expect(delegate.onScriptParsed).toHaveBeenCalledTimes(0);
  });

  it('immediately calls scriptParsed from END message', () => {
    const array = encodeArray(SP.JERRY_DEBUGGER_SOURCE_CODE_END, 'abc');
    // code = 'abc' + END
    handler.onSourceCode(array);
    expect(delegate.onScriptParsed).toHaveBeenCalledTimes(1);
    const data = delegate.onScriptParsed.mock.calls[0][0];
    // first script is #1, 'abc' is just one line, and no name was given
    expect(data.id).toEqual(1);
    expect(data.lineCount).toEqual(1);
    expect(data.name).toEqual('');
    expect(handler.getSource(1)).toEqual('abc');
  });

  it('concatenates multiple SOURCE messages with END message', () => {
    const array = encodeArray(SP.JERRY_DEBUGGER_SOURCE_CODE, 'abc');
    // code = 'abc' + 'abc' + 'abc'
    handler.onSourceCode(array);
    handler.onSourceCode(array);
    handler.onSourceCode(array);
    expect(delegate.onScriptParsed).toHaveBeenCalledTimes(0);
    array[0] = SP.JERRY_DEBUGGER_SOURCE_CODE_END;
    // code += 'abc' + END
    handler.onSourceCode(array);
    expect(delegate.onScriptParsed).toHaveBeenCalledTimes(1);
    // 'abcabcabc' + 'abc' = 'abcabcabcabc'
    expect(handler.getSource(1)).toEqual('abcabcabcabc');
  });
});

describe('onSourceCodeName', () => {
  const delegate = {
    onScriptParsed: jest.fn(),
  };
  let handler: JerryDebugProtocolHandler;

  beforeEach(() => {
    delegate.onScriptParsed.mockClear();
    handler = new JerryDebugProtocolHandler(delegate);
  });

  it('immediately completes name from END message', () => {
    // name = 'foo' + END
    let array = encodeArray(SP.JERRY_DEBUGGER_SOURCE_CODE_NAME_END, 'foo');
    handler.onSourceCodeName(array);
    // source = 'abc' + END
    array = encodeArray(SP.JERRY_DEBUGGER_SOURCE_CODE_END, 'abc');
    handler.onSourceCode(array);
    expect(delegate.onScriptParsed).toHaveBeenCalledTimes(1);
    const data = delegate.onScriptParsed.mock.calls[0][0];
    expect(data.name).toEqual('foo');
  });

  it('concatenates multiple NAME messages with END message', () => {
    // name = 'foo'
    let array = encodeArray(SP.JERRY_DEBUGGER_SOURCE_CODE_NAME, 'foo');
    handler.onSourceCodeName(array);
    // name += 'foo' + END
    array[0] = SP.JERRY_DEBUGGER_SOURCE_CODE_NAME_END;
    handler.onSourceCodeName(array);
    // source = 'abc' + END
    array = encodeArray(SP.JERRY_DEBUGGER_SOURCE_CODE_END, 'abc');
    handler.onSourceCode(array);
    expect(delegate.onScriptParsed).toHaveBeenCalledTimes(1);
    const data = delegate.onScriptParsed.mock.calls[0][0];
    // 'foo' + 'foo' = 'foofoo'
    expect(data.name).toEqual('foofoo');
  });
});

describe('releaseFunction', () => {
  it('updates functions, lineLists, and activeBreakpoints', () => {
    const byteCodeCP = 0;
    const func = {
      scriptId: 7,
      lines: [
        { activeIndex: 3 },
        { activeIndex: -1 },
        { activeIndex: -1 },
      ],
    };
    const handler = new JerryDebugProtocolHandler({});
    (handler as any).functions = [ func ];
    (handler as any).lineLists = {
      7: [[func], ['a', func], [func, 'b']],
    };
    (handler as any).activeBreakpoints = [1, 2, 3, 4, 5];
    handler.releaseFunction(byteCodeCP);
    expect((handler as any).activeBreakpoints[3]).toEqual(undefined);
    expect((handler as any).functions[byteCodeCP]).toEqual(undefined);
    expect((handler as any).lineLists[7]).toEqual([ [], [ 'a' ], [ 'b' ] ]);
  });
});

describe('onBreakpointHit', () => {
  it('calls delegate function if available', () => {
    const delegate = {
      onBreakpointHit: jest.fn(),
    };
    const handler = new JerryDebugProtocolHandler(delegate);

    let array = Uint8Array.from([0, 128, 2, 1, 1]);
    handler.onConfiguration(array);
    array = encodeArray(SP.JERRY_DEBUGGER_SOURCE_CODE_END, 'code');
    handler.onSourceCode(array);
    array = Uint8Array.from([SP.JERRY_DEBUGGER_BREAKPOINT_LIST, 25, 0, 0, 0]);
    handler.onBreakpointList(array);
    array = Uint8Array.from([SP.JERRY_DEBUGGER_BREAKPOINT_OFFSET_LIST, 125, 0, 0, 0]);
    handler.onBreakpointList(array);
    array = Uint8Array.from([SP.JERRY_DEBUGGER_BYTE_CODE_CP, 42, 0]);
    handler.onByteCodeCP(array);
    array = Uint8Array.from([SP.JERRY_DEBUGGER_BREAKPOINT_HIT, 42, 0, 125, 0, 0, 0]);
    expect(delegate.onBreakpointHit).toHaveBeenCalledTimes(0);
    handler.onBreakpointHit(array);
    expect(delegate.onBreakpointHit).toHaveBeenCalledTimes(1);
  });
});

describe('onBacktrace', () => {
  const delegate = {
    onBacktrace: jest.fn(),
  };
  const handler = new JerryDebugProtocolHandler(delegate);

  beforeEach(() => {
    delegate.onBacktrace.mockClear();
  });

  it('calls delegate function immediately on END event', () => {
    let array = Uint8Array.from([0, 128, 2, 1, 1]);
    handler.onConfiguration(array);
    array = encodeArray(SP.JERRY_DEBUGGER_SOURCE_CODE_END, 'code');
    handler.onSourceCode(array);
    array = Uint8Array.from([SP.JERRY_DEBUGGER_BREAKPOINT_LIST,
      16, 0, 0, 0,
      25, 0, 0, 0]);
    handler.onBreakpointList(array);
    array = Uint8Array.from([SP.JERRY_DEBUGGER_BREAKPOINT_OFFSET_LIST,
      64, 0, 0, 0,
      125, 0, 0, 0]);
    handler.onBreakpointList(array);
    array = Uint8Array.from([SP.JERRY_DEBUGGER_BYTE_CODE_CP, 42, 0]);
    handler.onByteCodeCP(array);

    array = Uint8Array.from([SP.JERRY_DEBUGGER_BACKTRACE_END, 42, 0, 125, 0, 0, 0]);
    expect(delegate.onBacktrace).toHaveBeenCalledTimes(0);
    handler.onBacktrace(array);
    expect(delegate.onBacktrace).toHaveBeenCalledTimes(1);
  });

  it('calls delegate function only on END event', () => {
    let array = Uint8Array.from([0, 128, 2, 1, 1]);
    handler.onConfiguration(array);
    array = encodeArray(SP.JERRY_DEBUGGER_SOURCE_CODE_END, 'code');
    handler.onSourceCode(array);
    array = Uint8Array.from([SP.JERRY_DEBUGGER_BREAKPOINT_LIST,
      16, 0, 0, 0,
      25, 0, 0, 0]);
    handler.onBreakpointList(array);
    array = Uint8Array.from([SP.JERRY_DEBUGGER_BREAKPOINT_OFFSET_LIST,
      64, 0, 0, 0,
      125, 0, 0, 0]);
    handler.onBreakpointList(array);
    array = Uint8Array.from([SP.JERRY_DEBUGGER_BYTE_CODE_CP, 42, 0]);
    handler.onByteCodeCP(array);

    array = Uint8Array.from([SP.JERRY_DEBUGGER_BACKTRACE, 42, 0, 64, 0, 0, 0]);
    expect(delegate.onBacktrace).toHaveBeenCalledTimes(0);
    array = Uint8Array.from([SP.JERRY_DEBUGGER_BACKTRACE_END, 42, 0, 125, 0, 0, 0]);
    expect(delegate.onBacktrace).toHaveBeenCalledTimes(0);
    handler.onBacktrace(array);
    expect(delegate.onBacktrace).toHaveBeenCalledTimes(1);
  });
});

describe('onEvalResult', () => {
  it('handles a single END packet', () => {
    const delegate = {
      onEvalResult: jest.fn(),
    };
    const handler = new JerryDebugProtocolHandler(delegate);
    (handler as any).evalResultData = undefined;
    (handler as any).evalsPending = 1;
    handler.onEvalResult(Uint8Array.from([SP.JERRY_DEBUGGER_EVAL_RESULT_END,
      'a'.charCodeAt(0), 'b'.charCodeAt(0), SP.JERRY_DEBUGGER_EVAL_OK]));
    expect(delegate.onEvalResult).toHaveBeenCalledTimes(1);
    expect(delegate.onEvalResult.mock.calls[0][0]).toEqual(SP.JERRY_DEBUGGER_EVAL_OK);
    expect(delegate.onEvalResult.mock.calls[0][1]).toEqual('ab');
    expect((handler as any).evalResultData).toEqual(undefined);
    expect((handler as any).evalsPending).toEqual(0);
  });

  it('handles a partial packet plus an END packet', () => {
    const delegate = {
      onEvalResult: jest.fn(),
    };
    const handler = new JerryDebugProtocolHandler(delegate);
    (handler as any).evalResultData = undefined;
    (handler as any).evalsPending = 1;
    handler.onEvalResult(Uint8Array.from([SP.JERRY_DEBUGGER_EVAL_RESULT,
      'a'.charCodeAt(0), 'b'.charCodeAt(0)]));
    handler.onEvalResult(Uint8Array.from([SP.JERRY_DEBUGGER_EVAL_RESULT_END,
      'a'.charCodeAt(0), 'b'.charCodeAt(0), SP.JERRY_DEBUGGER_EVAL_OK]));
    expect(delegate.onEvalResult).toHaveBeenCalledTimes(1);
    expect(delegate.onEvalResult.mock.calls[0][0]).toEqual(SP.JERRY_DEBUGGER_EVAL_OK);
    expect(delegate.onEvalResult.mock.calls[0][1]).toEqual('abab');
    expect((handler as any).evalResultData).toEqual(undefined);
    expect((handler as any).evalsPending).toEqual(0);
  });
});

describe('onMessage', () => {
  const delegate = {
    onError: jest.fn(),
  };
  const handler = new JerryDebugProtocolHandler(delegate);

  beforeEach(() => {
    delegate.onError.mockClear();
  });

  it('aborts when message too short', () => {
    handler.onMessage(new Uint8Array(0));
    expect(delegate.onError).toHaveBeenCalledTimes(1);
  });

  it('aborts when first message is not configuration', () => {
    const array = Uint8Array.from([SP.JERRY_DEBUGGER_SOURCE_CODE_END, 1, 2, 3]);
    handler.onMessage(array);
    expect(delegate.onError).toHaveBeenCalledTimes(1);
  });

  it('aborts when unhandled message sent', () => {
    const array = Uint8Array.from([SP.JERRY_DEBUGGER_CONFIGURATION, 200, 4, 1, 2]);
    handler.onMessage(array);
    expect(delegate.onError).toHaveBeenCalledTimes(0);
    array[0] = 255;
    handler.onMessage(array);
    expect(delegate.onError).toHaveBeenCalledTimes(1);
  });
});

describe('getScriptIdByName', () => {
  it('throws if no sources', () => {
    const handler = new JerryDebugProtocolHandler({});
    expect(() => handler.getScriptIdByName('mozzarella')).toThrow();
  });

  it('throws if not match for source name', () => {
    const handler = new JerryDebugProtocolHandler({});
    let array = encodeArray(SP.JERRY_DEBUGGER_SOURCE_CODE_NAME_END, 'mozzarella');
    handler.onSourceCodeName(array);
    array = encodeArray(SP.JERRY_DEBUGGER_SOURCE_CODE_END, 'code');
    handler.onSourceCode(array);

    expect(() => handler.getScriptIdByName('pepperjack')).toThrow();
  });

  it('returns index match found for source name', () => {
    const handler = new JerryDebugProtocolHandler({});
    let array = encodeArray(SP.JERRY_DEBUGGER_SOURCE_CODE_NAME_END, 'mozzarella');
    handler.onSourceCodeName(array);
    array = encodeArray(SP.JERRY_DEBUGGER_SOURCE_CODE_END, 'code');
    handler.onSourceCode(array);

    // script IDs are 1-indexed
    expect(handler.getScriptIdByName('mozzarella')).toEqual(1);
  });
});

describe('evaluate', () => {
  it('sends single eval packet for short expressions', () => {
    const debugClient = {
      send: jest.fn(),
    };
    const handler = new JerryDebugProtocolHandler({});
    (handler as any).lastBreakpointHit = true;
    (handler as any).byteConfig = {
      littleEndian: true,
    };
    (handler as any).maxMessageSize = 16;
    (handler as any).debuggerClient = debugClient;
    handler.evaluate('foo');
    expect(debugClient.send).toHaveBeenCalledTimes(1);
    expect(debugClient.send.mock.calls[0][0]).toEqual(Uint8Array.from([
      SP.JERRY_DEBUGGER_EVAL, 3, 0, 0, 0,
      'f'.charCodeAt(0), 'o'.charCodeAt(0), 'o'.charCodeAt(0),
    ]));
  });

  it('sends two eval packets for longer expression', () => {
    const debugClient = {
      send: jest.fn(),
    };
    const handler = new JerryDebugProtocolHandler({});
    (handler as any).lastBreakpointHit = true;
    (handler as any).byteConfig = {
      littleEndian: true,
    };
    (handler as any).maxMessageSize = 6;
    (handler as any).debuggerClient = debugClient;
    handler.evaluate('foobar');
    expect(debugClient.send).toHaveBeenCalledTimes(2);
    expect(debugClient.send.mock.calls[0][0]).toEqual(Uint8Array.from([
      SP.JERRY_DEBUGGER_EVAL, 6, 0, 0, 0, 'f'.charCodeAt(0),
    ]));
    expect(debugClient.send.mock.calls[1][0]).toEqual(Uint8Array.from([
      SP.JERRY_DEBUGGER_EVAL_PART, 'o'.charCodeAt(0), 'o'.charCodeAt(0),
      'b'.charCodeAt(0), 'a'.charCodeAt(0), 'r'.charCodeAt(0),
    ]));
  });
});

describe('findBreakpoint', () => {
  it('throws on scriptId 0 with one source', () => {
    const handler = new JerryDebugProtocolHandler({});
    const array = encodeArray(SP.JERRY_DEBUGGER_SOURCE_CODE_END, 'code');
    handler.onSourceCode(array);
    expect(() => handler.findBreakpoint(0, 5)).toThrow();
  });

  it('throws on scriptId 2 with one source', () => {
    const handler = new JerryDebugProtocolHandler({});
    const array = encodeArray(SP.JERRY_DEBUGGER_SOURCE_CODE_END, 'code');
    handler.onSourceCode(array);
    expect(() => handler.findBreakpoint(2, 5)).toThrow();
  });

  it('throws on line w/o breakpoint, succeeds on line w/ breakpoint', () => {
    const handler = new JerryDebugProtocolHandler({});
    let array = Uint8Array.from([0, 128, 2, 1, 1]);
    handler.onConfiguration(array);

    array = encodeArray(SP.JERRY_DEBUGGER_SOURCE_CODE_END, 'code');
    handler.onSourceCode(array);

    array = Uint8Array.from([SP.JERRY_DEBUGGER_BREAKPOINT_LIST,
      4, 0, 0, 0,
      9, 0, 0, 0,
      16, 0, 0, 0,
      25, 0, 0, 0]);
    handler.onBreakpointList(array);
    array = Uint8Array.from([SP.JERRY_DEBUGGER_BREAKPOINT_OFFSET_LIST,
      8, 0, 0, 0,
      27, 0, 0, 0,
      64, 0, 0, 0,
      125, 0, 0, 0]);
    handler.onBreakpointList(array);

    array = Uint8Array.from([SP.JERRY_DEBUGGER_BYTE_CODE_CP, 42, 0]);
    handler.onByteCodeCP(array);
    expect(() => handler.findBreakpoint(1, 6)).toThrow();
    expect(handler.findBreakpoint(1, 4).line).toEqual(4);
  });
});

describe('updateBreakpoint', () => {
  const debugClient = {
    send: jest.fn(),
  };

  beforeEach(() => {
    debugClient.send.mockClear();
  });

  it('throws on enabling active breakpoint', () => {
    const bp: any = { activeIndex: 3 };
    const handler = new JerryDebugProtocolHandler({});
    expect(() => { handler.updateBreakpoint(bp, true); }).toThrowError('breakpoint already enabled');
  });

  it('throws on disabling inactive breakpoint', () => {
    const bp: any = { activeIndex: -1 };
    const handler = new JerryDebugProtocolHandler({});
    expect(() => { handler.updateBreakpoint(bp, false); }).toThrowError('breakpoint already disabled');
  });

  it('enables inactive breakpoint successfully', () => {
    const handler = new JerryDebugProtocolHandler({});
    let array = Uint8Array.from([0, 128, 2, 1, 1]);
    handler.onConfiguration(array);
    handler.debuggerClient = debugClient as any;

    const bp: any = {
      activeIndex: -1,
      func: {
        byteCodeCP: 42,
      },
      offset: 10,
    };
    expect(handler.updateBreakpoint(bp, true)).toEqual(bp.activeIndex);
    expect(bp.activeIndex).not.toEqual(-1);
    expect(debugClient.send).toHaveBeenCalledTimes(1);
  });

  it('disables active breakpoint successfully', () => {
    const handler = new JerryDebugProtocolHandler({});
    let array = Uint8Array.from([0, 128, 2, 1, 1]);
    handler.onConfiguration(array);
    handler.debuggerClient = debugClient as any;

    const bp: any = {
      activeIndex: 4,
      func: {
        byteCodeCP: 42,
      },
      offset: 10,
    };
    expect(handler.updateBreakpoint(bp, false)).toEqual(4);
    expect(bp.activeIndex).toEqual(-1);
    expect(debugClient.send).toHaveBeenCalledTimes(1);
  });
});

describe('requestBacktrace', () => {
  const debugClient = {
    send: jest.fn(),
  };

  beforeEach(() => {
    debugClient.send.mockClear();
  });

  it('throws if not at a breakpoint', () => {
    const handler = new JerryDebugProtocolHandler({});
    expect(() => handler.requestBacktrace()).toThrow();
  });

  it('sends if at a breakpoint', () => {
    const handler = new JerryDebugProtocolHandler({});
    handler.debuggerClient = debugClient as any;

    let array = Uint8Array.from([0, 128, 2, 1, 1]);
    handler.onConfiguration(array);
    array = encodeArray(SP.JERRY_DEBUGGER_SOURCE_CODE_END, 'code');
    handler.onSourceCode(array);
    array = Uint8Array.from([SP.JERRY_DEBUGGER_BREAKPOINT_LIST, 25, 0, 0, 0]);
    handler.onBreakpointList(array);
    array = Uint8Array.from([SP.JERRY_DEBUGGER_BREAKPOINT_OFFSET_LIST, 125, 0, 0, 0]);
    handler.onBreakpointList(array);
    array = Uint8Array.from([SP.JERRY_DEBUGGER_BYTE_CODE_CP, 42, 0]);
    handler.onByteCodeCP(array);
    array = Uint8Array.from([SP.JERRY_DEBUGGER_BREAKPOINT_HIT, 42, 0, 125, 0, 0, 0]);
    handler.onBreakpointHit(array);
    expect(debugClient.send).toHaveBeenCalledTimes(0);
    handler.requestBacktrace();
    expect(debugClient.send).toHaveBeenCalledTimes(1);
  });
});

describe('stepping', () => {
  it('sends the expected message when calling stepInto()', () => {
    const { handler, debugClient } = setupHaltedProtocolHandler();
    handler.stepInto();
    expect(debugClient.send).toHaveBeenCalledWith(Uint8Array.from([SP.JERRY_DEBUGGER_STEP]));
  });

  it('sends the expected message when calling stepOut()', () => {
    const { handler, debugClient } = setupHaltedProtocolHandler();
    handler.stepOut();
    expect(debugClient.send).toHaveBeenCalledWith(Uint8Array.from([SP.JERRY_DEBUGGER_FINISH]));
  });

  it('sends the expected message when calling stepOver()', () => {
    const { handler, debugClient } = setupHaltedProtocolHandler();
    handler.stepOver();
    expect(debugClient.send).toHaveBeenCalledWith(Uint8Array.from([SP.JERRY_DEBUGGER_NEXT]));
  });
});

describe('logPacket', () => {
  it('prints packet name in brackets without second arg', () => {
    const spy = jest.spyOn(console, 'log');
    const handler = new JerryDebugProtocolHandler({});
    handler.logPacket('foo');
    expect(spy).toHaveBeenCalled();
    expect(spy.mock.calls[0][0]).toEqual('[foo]');
  });

  it('prints packet name in brackets with second arg false', () => {
    const spy = jest.spyOn(console, 'log');
    const handler = new JerryDebugProtocolHandler({});
    handler.logPacket('foo', false);
    expect(spy).toHaveBeenCalled();
    expect(spy.mock.calls[0][0]).toEqual('[foo]');
  });

  it('prints packet name in brackets with second arg true', () => {
    const spy = jest.spyOn(console, 'log');
    const handler = new JerryDebugProtocolHandler({});
    (handler as any).evalsPending = 1;
    handler.logPacket('foo', true);
    expect(spy).toHaveBeenCalled();
    expect(spy.mock.calls[0][0]).toEqual('[Ignored: foo]');
  });
});
