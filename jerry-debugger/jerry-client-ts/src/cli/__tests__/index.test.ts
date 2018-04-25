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

import { getOptionsFromArgs } from '../cli';
import {
  DEFAULT_DEBUGGER_HOST,
  DEFAULT_DEBUGGER_PORT,
} from '../../lib/debugger-client';
import {
  DEFAULT_SERVER_HOST,
  DEFAULT_SERVER_PORT,
} from '../../lib/cdt-proxy';

describe('getOptionsFromArgs', () => {

  function getOptionsFromUserArgs(userArgs: string[]) {
    return getOptionsFromArgs(['node', 'jerry-debugger.js'].concat(userArgs));
  }

  it('works without --inspect-brk', () => {
    const opt = getOptionsFromUserArgs([]);
    expect(opt.proxyAddress.host).toEqual(DEFAULT_SERVER_HOST);
    expect(opt.proxyAddress.port).toEqual(DEFAULT_SERVER_PORT);
  });

  it('parses --inspect-brk with port only', () => {
    const opt = getOptionsFromUserArgs(['--inspect-brk=1234']);
    expect(opt.proxyAddress.host).toEqual(undefined);
    expect(opt.proxyAddress.port).toEqual(1234);
  });

  it('parses --inspect-brk with no port', () => {
    const opt = getOptionsFromUserArgs(['--inspect-brk=10.10.10.10:']);
    expect(opt.proxyAddress.host).toEqual('10.10.10.10');
    expect(opt.proxyAddress.port).toEqual(undefined);
  });

  it('parses --inspect-brk with no host', () => {
    const opt = getOptionsFromUserArgs(['--inspect-brk=:1234']);
    expect(opt.proxyAddress.host).toEqual(undefined);
    expect(opt.proxyAddress.port).toEqual(1234);
  });

  it('parses --inspect-brk with host and port', () => {
    const opt = getOptionsFromUserArgs(['--inspect-brk=10.10.10.10:1234']);
    expect(opt.proxyAddress.host).toEqual('10.10.10.10');
    expect(opt.proxyAddress.port).toEqual(1234);
  });

  it('works without --jerry-remote', () => {
    const opt = getOptionsFromUserArgs([]);
    expect(opt.remoteAddress.host).toEqual(DEFAULT_DEBUGGER_HOST);
    expect(opt.remoteAddress.port).toEqual(DEFAULT_DEBUGGER_PORT);
  });

  it('parses --jerry-remote with port only', () => {
    const opt = getOptionsFromUserArgs(['--jerry-remote=1234']);
    expect(opt.remoteAddress.host).toEqual(undefined);
    expect(opt.remoteAddress.port).toEqual(1234);
  });

  it('parses --jerry-remote with host and port', () => {
    const opt = getOptionsFromUserArgs(['--jerry-remote=10.10.10.10:1234']);
    expect(opt.remoteAddress.host).toEqual('10.10.10.10');
    expect(opt.remoteAddress.port).toEqual(1234);
  });

  it('verbose defaults to false', () => {
    const opt = getOptionsFromUserArgs([]);
    expect(opt.verbose).toEqual(false);
  });

  it('parses verbose flag', () => {
    const opt = getOptionsFromUserArgs(['--verbose']);
    expect(opt.verbose).toEqual(true);
  });

  it('parses v alias for verbose', () => {
    const opt = getOptionsFromUserArgs(['-v']);
    expect(opt.verbose).toEqual(true);
  });

  it('jsfile defaults to untitled.js', () => {
    const opt = getOptionsFromUserArgs([]);
    expect(opt.jsfile).toEqual('untitled.js');
  });

  it('returns client source as jsfile', () => {
    const opt = getOptionsFromUserArgs(['foo/bar.js']);
    expect(opt.jsfile).toEqual('foo/bar.js');
  });

});
