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

import { Breakpoint } from '../breakpoint';
import { CDTController } from '../cdt-controller';
import { ChromeDevToolsProxyServer } from '../cdt-proxy';
import { JERRY_DEBUGGER_EVAL_OK, JERRY_DEBUGGER_EVAL_ERROR } from '../jrs-protocol-constants';

describe('onError', () => {
  const spy = jest.spyOn(console, 'log');

  it('prints message and code', () => {
    const controller = new CDTController();
    controller.onError(42, 'foo');
    expect(spy).toHaveBeenCalled();
    expect(spy.mock.calls[0][0]).toEqual('Error: foo (42)');
  });
});

describe('onScriptParsed', () => {
  const controller = new CDTController();
  const server = {
    scriptParsed: jest.fn(),
  };

  beforeEach(() => {
    controller.proxyServer = undefined;
    server.scriptParsed.mockClear();
  });

  it('does nothing if proxyServer not set', () => {
    controller.onScriptParsed({
      id: 42,
      name: 'cheese.js',
      lineCount: 3,
    });
    expect(server.scriptParsed).toHaveBeenCalledTimes(0);
  });

  it('reports script parsed if proxyServer set', () => {
    controller.proxyServer = server as any;
    controller.onScriptParsed({
      id: 42,
      name: 'cheese.js',
      lineCount: 3,
    });
    expect(server.scriptParsed).toHaveBeenCalledTimes(1);
  });
});

describe('onBreakpointHit', () => {
  const controller = new CDTController();
  const handler = {
    requestBacktrace: jest.fn(),
  };
  controller.protocolHandler = handler as any;

  beforeEach(() => {
    controller.proxyServer = undefined;
    handler.requestBacktrace.mockClear();
  });

  it('does nothing if proxyServer not set', () => {
    controller.onBreakpointHit({
      breakpoint: {} as Breakpoint,
      exact: false,
    });
    expect(handler.requestBacktrace).toHaveBeenCalledTimes(0);
  });

  it('requests backtrace if proxyServer set', () => {
    controller.proxyServer = {} as ChromeDevToolsProxyServer;
    controller.onBreakpointHit({
      breakpoint: {} as Breakpoint,
      exact: false,
    });
    expect(handler.requestBacktrace).toHaveBeenCalledTimes(1);
  });
});

describe('onEvalResult', () => {
  const controller = new CDTController();
  const handler = {
    evaluate: jest.fn(),
  };
  controller.protocolHandler = handler as any;

  beforeEach(() => {
    controller.proxyServer = undefined;
    handler.evaluate.mockClear();
  });

  it('throws if run with no eval pending', () => {
    expect(() => controller.onEvalResult(JERRY_DEBUGGER_EVAL_OK, 'foo')).toThrow();
  });

  it('calls resolve for successful eval', () => {
    const promise = controller.cmdEvaluateOnCallFrame({
      callFrameId: '0',
      expression: 'valid',
    });
    controller.onEvalResult(JERRY_DEBUGGER_EVAL_OK, 'success');
    return expect(promise).resolves.toEqual({
      result: {
        type: 'string',
        value: 'success',
      },
    });
  });

  it('calls reject for failed eval', () => {
    const promise = controller.cmdEvaluateOnCallFrame({
      callFrameId: '0',
      expression: 'invalid',
    });
    controller.onEvalResult(JERRY_DEBUGGER_EVAL_ERROR, 'failure');
    return expect(promise).rejects.toEqual({
      result: {
        type: 'string',
        value: 'failure',
      },
    });
  });
});

describe('requestScripts', () => {
  it('sends scriptParsed event to server for all known scripts', () => {
    const controller = new CDTController();
    // queue up two scripts before server is connected
    controller.onScriptParsed({
      id: 1,
      name: 'gruyere.js',
      lineCount: 3,
    });
    controller.onScriptParsed({
      id: 2,
      name: 'wensleydale.js',
      lineCount: 5,
    });
    const server = {
      scriptParsed: jest.fn(),
    };
    controller.proxyServer = server as any;
    expect(server.scriptParsed).toHaveBeenCalledTimes(0);
    controller.requestScripts();
    expect(server.scriptParsed).toHaveBeenCalledTimes(2);
  });
});

describe('requestBreakpoint', () => {
  it('throws if not at a breakpoint', () => {
    const controller = new CDTController();
    controller.protocolHandler = {
      getLastBreakpoint: () => undefined,
    } as any;
    expect(() => controller.requestBreakpoint()).toThrow();
  });

  it('requests a backtrace if at a breakpoint', () => {
    const controller = new CDTController();
    const handler = {
      getLastBreakpoint: () => { return {} as Breakpoint; },
      requestBacktrace: jest.fn(),
    };
    controller.protocolHandler = handler as any;
    expect(handler.requestBacktrace).toHaveBeenCalledTimes(0);
    controller.requestBreakpoint();
    expect(handler.requestBacktrace).toHaveBeenCalledTimes(1);
  });
});

describe('requestPossibleBreakpoints', () => {
  it('returns an array of locations', async () => {
    const controller = new CDTController();
    const handler = {
      getPossibleBreakpoints: (scriptId: number, startLine: number, endLine: number) => {
        const bp1 = {
          line: 100,
        } as Breakpoint;
        const bp2 = {
          line: 200,
        } as Breakpoint;
        return [bp1, bp2];
      },
    };
    controller.protocolHandler = handler as any;

    const obj = await controller.requestPossibleBreakpoints({
      start: {
        scriptId: '1',
        lineNumber: 50,
      },
    });

    expect(obj.locations.length).toEqual(2);
    expect(obj.locations[0].scriptId).toEqual('1');
    expect(obj.locations[0].lineNumber).toEqual(99);
    expect(obj.locations[1].scriptId).toEqual('1');
    expect(obj.locations[1].lineNumber).toEqual(199);
  });
});
