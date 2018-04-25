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

import { onHttpRequest } from '../cdt-proxy-http';

describe('onHttpRequest', () => {
  // common setup
  const proxy = {
    host: '127.0.0.1',
    port: 9229,
    jsfile: 'foo/bar/baz.js',
  };
  const request = {
    method: 'GET',
    url: '',
  };
  const response = {
    statusCode: 200,
    setHeader: jest.fn(),
    end: jest.fn(),
    write: jest.fn(),
  };

  // clean up before each test
  beforeEach(() => {
    request.method = 'GET';
    response.statusCode = 200;
  });

  it('responds to POST with 405', () => {
    request.method = 'POST';
    onHttpRequest.call(proxy, request, response);
    expect(response.statusCode).toEqual(405);
    expect(response.end).toBeCalled();
  });

  it('responds to unexpected path with 404', () => {
    request.url = '/foo';
    onHttpRequest.call(proxy, request, response);
    expect(response.statusCode).toEqual(404);
    expect(response.end).toBeCalled();
  });

  it('responds to version query with JSON', () => {
    request.url = '/json/version';
    onHttpRequest.call(proxy, request, response);
    expect(response.statusCode).toEqual(200);
    expect(response.end).toBeCalled();
    expect(response.write).toHaveBeenCalled();

    const obj = JSON.parse(response.write.mock.calls[0][0]);
    expect(obj['Browser']).toBeDefined();
    expect(obj['Protocol-Version']).toBeDefined();
  });

  it('responds to /json query with JSON', () => {
    request.url = '/json';
    onHttpRequest.call(proxy, request, response);
    expect(response.statusCode).toEqual(200);
    expect(response.end).toBeCalled();
    expect(response.write).toHaveBeenCalled();

    const json = response.write.mock.calls[0][0];
    const array = JSON.parse(json);
    expect(array).toHaveLength(1);
    expect(array[0].description).toBeDefined();
    expect(array[0].devtoolsFrontendUrl).toBeDefined();
    expect(array[0].type).toBeDefined();
    expect(array[0].webSocketDebuggerUrl).toBeDefined();
  });

  it('responds to list query with the same JSON', () => {
    request.url = '/json';
    onHttpRequest.call(proxy, request, response);
    const json = response.write.mock.calls[0][0];

    request.url = '/json/list';
    onHttpRequest.call(proxy, request, response);
    expect(response.statusCode).toEqual(200);
    expect(response.end).toBeCalled();
    expect(response.write).toHaveBeenCalled();
    expect(response.write.mock.calls[0][0]).toEqual(json);
  });

  it('responds to unexpected json query with 404', () => {
    request.url = '/json/foo';
    onHttpRequest.call(proxy, request, response);
    expect(response.statusCode).toEqual(404);
    expect(response.end).toBeCalled();
  });
});
