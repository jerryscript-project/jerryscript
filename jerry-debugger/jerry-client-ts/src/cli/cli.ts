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

import { CDTController } from '../lib/cdt-controller';
import {
  ChromeDevToolsProxyServer,
  DEFAULT_SERVER_HOST,
  DEFAULT_SERVER_PORT,
} from '../lib/cdt-proxy';
import {
  JerryDebuggerClient,
  DEFAULT_DEBUGGER_HOST,
  DEFAULT_DEBUGGER_PORT,
} from '../lib/debugger-client';
import { Command } from 'commander';
import { JerryDebugProtocolHandler } from '../lib/protocol-handler';

/**
 * Converts string of format [host:][port] to an object with host and port,
 * each possibly undefined
 */
function getHostAndPort(input: string) {
  const hostAndPort = input.split(':');
  const portIndex = hostAndPort.length - 1;
  const host = hostAndPort[portIndex - 1];
  const port = hostAndPort[portIndex];
  return {
    host: host ? host : undefined,
    port: port ? Number(port) : undefined,
  };
}

export function getOptionsFromArgs(argv: Array<string>) {
  const program = new Command('jerry-debugger');
  program
    .usage('[options] <script.js ...>')
    .option(
      '-v, --verbose',
      'Enable verbose logging', false)
    .option(
      '--inspect-brk [[host:]port]',
      'Activate Chrome DevTools proxy on host:port',
      `${DEFAULT_SERVER_HOST}:${DEFAULT_SERVER_PORT}`)
    .option(
      '--jerry-remote [[host:]port]',
      'Connect to JerryScript on host:port',
      `${DEFAULT_DEBUGGER_HOST}:${DEFAULT_DEBUGGER_PORT}`)
    .parse(argv);

  return {
    proxyAddress: getHostAndPort(program.inspectBrk),
    remoteAddress: getHostAndPort(program.jerryRemote),
    jsfile: program.args[0] || 'untitled.js',
    verbose: program.verbose || false,
  };
}

export function main(proc: NodeJS.Process) {
  const options = getOptionsFromArgs(proc.argv);

  const controller = new CDTController();
  const jhandler = new JerryDebugProtocolHandler(controller);
  const jclient = new JerryDebuggerClient({
    delegate: jhandler,
    ...options.remoteAddress,
  });
  jhandler.debuggerClient = jclient;
  // set this before connecting to the client
  controller.protocolHandler = jhandler;

  const debuggerUrl = `ws://${jclient.host}:${jclient.port}`;
  jclient.connect().then(() => {
    console.log(`Connected to debugger at ${debuggerUrl}`);
    const proxy = new ChromeDevToolsProxyServer({
      delegate: controller,
      ...options.proxyAddress,
      jsfile: options.jsfile,
    });
    // set this before making further debugger calls
    controller.proxyServer = proxy;
    console.log(`Proxy listening at ws://${proxy.host}:${proxy.port}`);
  }).catch((err) => {
    console.log(`Error connecting to debugger at ${debuggerUrl}`);
    console.log(err);
  });
}
