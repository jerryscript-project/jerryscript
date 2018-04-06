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

import * as WebSocket from 'ws';
import WebSocketServer = WebSocket.Server;
import * as rpc from 'noice-json-rpc';
import Crdp from 'chrome-remote-debug-protocol';
import * as http from 'http';
import uuid from 'uuid/v1';

import { Breakpoint } from './breakpoint';
import { onHttpRequest } from './cdt-proxy-http';
import { JerryMessageScriptParsed } from './protocol-handler';

export interface CDTDelegate {
  requestScripts: () => void;
  requestBreakpoint: () => void;
  requestPossibleBreakpoints: (request: Crdp.Debugger.GetPossibleBreakpointsRequest) =>
    Promise<Crdp.Debugger.GetPossibleBreakpointsResponse>;
  requestScriptSource: (request: Crdp.Debugger.GetScriptSourceRequest) =>
    Promise<Crdp.Debugger.GetScriptSourceResponse>;
  cmdEvaluateOnCallFrame: (request: Crdp.Debugger.EvaluateOnCallFrameRequest) =>
    Promise<Crdp.Debugger.EvaluateOnCallFrameResponse>;
  cmdPause: () => void;
  cmdRemoveBreakpoint: (breakpointId: number) => Promise<void>;
  cmdResume: () => void;
  cmdSetBreakpoint: (request: Crdp.Debugger.SetBreakpointRequest) =>
    Promise<Crdp.Debugger.SetBreakpointResponse>;
  cmdSetBreakpointByUrl: (request: Crdp.Debugger.SetBreakpointByUrlRequest) =>
    Promise<Crdp.Debugger.SetBreakpointByUrlResponse>;
  cmdStepInto: () => void;
  cmdStepOut: () => void;
  cmdStepOver: () => void;
}

export interface ChromeDevToolsProxyServerOptions {
  delegate: CDTDelegate;
  port?: number;
  host?: string;
  uuid?: string;
  jsfile?: string;
}

export const DEFAULT_SERVER_HOST = 'localhost';
export const DEFAULT_SERVER_PORT = 9229;
export const JERRY_DEBUGGER_VERSION = 'jerry-debugger/v0.0.1';
export const DEVTOOLS_PROTOCOL_VERSION = '1.1';

export class ChromeDevToolsProxyServer {
  readonly host: string;
  readonly port: number;
  readonly uuid: string;
  readonly jsfile: string;
  private skipAllPauses: boolean = false;
  private pauseOnExceptions: ('none' | 'uncaught' | 'all') = 'none';
  private asyncCallStackDepth: number = 0;  // 0 is unlimited
  private delegate: CDTDelegate;
  private api: Crdp.CrdpServer;
  private lastRuntimeScript = 1;

  constructor(options: ChromeDevToolsProxyServerOptions) {
    this.delegate = options.delegate;

    const server = http.createServer();

    this.host = options.host || DEFAULT_SERVER_HOST;
    this.port = options.port || DEFAULT_SERVER_PORT;
    this.uuid = options.uuid || uuid();
    // FIXME: probably not quite right, can include ../.. etc.
    this.jsfile = options.jsfile || 'untitled.js';

    server.listen(this.port);

    const wss = new WebSocketServer({ server });
    const rpcServer = new rpc.Server(wss);
    this.api = rpcServer.api();

    wss.on('connection', (ws, req) => {
      const ip = req.connection.remoteAddress;
      console.log(`connection from: ${ip}`);
    });

    server.on('request', onHttpRequest.bind(this));

    rpcServer.setLogging({
      logEmit: true,
      logConsole: true,
    });

    // Based on the example from https://github.com/nojvek/noice-json-rpc
    const notImplemented = async () => {
      console.log('Function not implemented');
    };

    this.api.Debugger.expose({
      enable: notImplemented,
      evaluateOnCallFrame: request => this.delegate.cmdEvaluateOnCallFrame(request),
      setSkipAllPauses: async (params) => {
        this.skipAllPauses = params.skip;
      },
      setBlackboxPatterns: notImplemented,
      getPossibleBreakpoints: request => this.delegate.requestPossibleBreakpoints(request),
      getScriptSource: request => this.delegate.requestScriptSource(request),
      pause: async () => this.delegate.cmdPause(),
      removeBreakpoint: request => this.delegate.cmdRemoveBreakpoint(Number(request.breakpointId)),
      resume: async () => this.delegate.cmdResume(),
      setPauseOnExceptions: async (params) => {
        this.pauseOnExceptions = params.state;
      },
      setAsyncCallStackDepth: async (params) => {
        this.asyncCallStackDepth = params.maxDepth;
      },
      setBreakpoint: request => this.delegate.cmdSetBreakpoint(request),
      setBreakpointByUrl: request => this.delegate.cmdSetBreakpointByUrl(request),
      stepInto: async () => this.delegate.cmdStepInto(),
      stepOut: async () => this.delegate.cmdStepOut(),
      stepOver: async () => this.delegate.cmdStepOver(),
    });
    this.api.Profiler.expose({ enable: notImplemented });
    this.api.Runtime.expose({
      enable: notImplemented,
      runIfWaitingForDebugger: async () => {
        // how could i chain this to happen after the enable response goes out?
        this.api.Runtime.emitExecutionContextCreated({
          context: {
            // might need to track multiple someday
            id: 1,
            origin: '',
            // node seems to use node[<PID>] FWIW
            name: 'jerryscript',
          },
        });

        // request controller to send scriptParsed event for existing scripts
        this.delegate.requestScripts();
        this.delegate.requestBreakpoint();
      },
      async runScript() {
        console.log('runScript called!');
        return {
          // Return a bogus result
          result: {
            type: 'boolean',
            value: true,
          },
        };
      },
    });
  }

  unused() {
    // FIXME: pretend to use these to get around lint error for now
    this.skipAllPauses, this.pauseOnExceptions, this.asyncCallStackDepth;
  }

  scriptParsed(script: JerryMessageScriptParsed) {
    let name = script.name;
    if (!name) {
      // FIXME: make file / module name available to use here
      name = `untitled${this.lastRuntimeScript++}`;
    }
    this.api.Debugger.emitScriptParsed({
      scriptId: String(script.id),
      url: name,
      startLine: 0,
      startColumn: 0,
      endLine: script.lineCount,
      endColumn: 0,
      executionContextId: 1,
      hash: '',
    });
  }

  /**
   * Sends Debugger.paused event for the current debugger location
   */
  sendPaused(breakpoint: Breakpoint | undefined, backtrace: Array<Breakpoint>,
             reason: 'exception' | 'debugCommand' | 'other') {
    const callFrames: Array<Crdp.Debugger.CallFrame> = [];
    let nextFrameId = 0;
    for (const bp of backtrace) {
      callFrames.push({
        callFrameId: String(nextFrameId++),
        functionName: bp.func.name,
        location: {
          scriptId: String(bp.func.scriptId),
          lineNumber: bp.line - 1,  // switch to 0-based
        },
        scopeChain: [],
        this: {
          type: 'object',
        },
      });
    }

    const hitBreakpoints = [];
    if (breakpoint) {
      hitBreakpoints.push(String(breakpoint.activeIndex));
    }

    this.api.Debugger.emitPaused({
      hitBreakpoints,
      reason,
      callFrames,
    });
  }

  sendResumed() {
    this.api.Debugger.emitResumed();
  }
}
