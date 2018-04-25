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

import WebSocket from 'ws';

export interface JerryDebuggerOptions {
  delegate: JerryDebuggerDelegate;
  host?: string;
  port?: number;
}

export interface JerryDebuggerDelegate {
  onMessage: (message: Uint8Array) => void;
}

export const DEFAULT_DEBUGGER_HOST = 'localhost';
export const DEFAULT_DEBUGGER_PORT = 5001;

export class JerryDebuggerClient {
  readonly host: string;
  readonly port: number;
  private socket?: WebSocket;
  private connectPromise?: Promise<void>;
  private delegate: JerryDebuggerDelegate;

  constructor(options: JerryDebuggerOptions) {
    this.delegate = options.delegate;
    this.host = options.host || DEFAULT_DEBUGGER_HOST;
    this.port = options.port || DEFAULT_DEBUGGER_PORT;
  }

  connect(): Promise<void> {
    if (this.connectPromise) {
      return this.connectPromise;
    }

    this.socket = new WebSocket(`ws://${this.host}:${this.port}/jerry-debugger`);
    this.socket.binaryType = 'arraybuffer';
    this.socket.on('message', this.onMessage.bind(this));

    this.connectPromise = new Promise((resolve, reject) => {
      if (!this.socket) {
        reject(new Error('socket missing'));
        return;
      }

      this.socket.on('open', () => {
        resolve();
      });

      this.socket.on('error', (err) => {
        reject(err);
      });
    });

    return this.connectPromise;
  }

  disconnect() {
    if (this.socket) {
      this.socket.terminate();
      this.socket = undefined;
    }
  }

  onMessage(data: ArrayBuffer) {
    this.delegate.onMessage(new Uint8Array(data));
  }

  send(data: ArrayBuffer) {
    this.socket!.send(data);
  }
}
