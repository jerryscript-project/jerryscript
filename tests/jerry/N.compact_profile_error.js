// Copyright 2014 Samsung Electronics Co., Ltd.
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

var catched = false;

function f1()
{
  var arguments = 1;
}

try
{
  f1();
} catch (e)
{
  assert (e === CompactProfileError);

  catched = true;
}

assert(catched);

catched = false;

function f2()
{
  var a = arguments;
}

try
{
  f2();
} catch (e)
{
  assert (e === CompactProfileError);

  catched = true;
}

assert(catched);

catched = false;

try
{
  eval('abc');
} catch (e)
{
  assert (e === CompactProfileError);

  catched = true;
}

assert(catched);

catched = false;

try
{
  Function('abc');
} catch (e)
{
  assert (e === CompactProfileError);

  catched = true;
}

assert(catched);

catched = false;

try
{
  new Function('abc');
} catch (e)
{
  assert (e === CompactProfileError);

  catched = true;
}

assert(catched);

catched = false;

try
{
  var a = Date.now();
} catch (e)
{
  assert (e === CompactProfileError);

  catched = true;
}

assert(catched);
