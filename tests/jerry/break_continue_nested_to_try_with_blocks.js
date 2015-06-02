// Copyright 2015 Samsung Electronics Co., Ltd.
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

/* with */

for (var i = 0; i < 10; i++)
{
  with ({})
  {
    break;

    assert (false);
  }
}
assert (i === 0);

for (var i = 0; i < 10; i++)
{
  with ({})
  {
    continue;

    assert (false);
  }
}
assert (i === 10);

/* try */
for (var i = 0; i < 10; i++)
{
  try
  {
    break;

    assert (false);
  }
  catch (e)
  {
  }
}
assert (i === 0);

for (var i = 0; i < 10; i++)
{
  try
  {
    continue;

    assert (false);
  }
  catch (e)
  {
  }
}
assert (i === 10);

/* catch */
for (var i = 0; i < 10; i++)
{
  try
  {
    throw new TypeError ();
    assert (false);
  }
  catch (e)
  {
    break;
    assert (false);
  }
}
assert (i === 0);

for (var i = 0; i < 10; i++)
{
  try
  {
    throw new TypeError ();
    assert (false);
  }
  catch (e)
  {
    continue;
    assert (false);
  }
}
assert (i === 10);


/* finally */
for (var i = 0; i < 10; i++)
{
  try
  {
    throw new TypeError ();
    assert (false);
  }
  catch (e)
  {
  }
  finally
  {
    break;
    assert (false);
  }
}
assert (i === 0);

for (var i = 0; i < 10; i++)
{
  try
  {
    throw new TypeError ();
    assert (false);
  }
  catch (e)
  {
  }
  finally
  {
    continue;
    assert (false);
  }
}
assert (i === 10);


/* with - switch */

str = '';
for (var i = 0; i < 10; i++)
{
  with ({})
  {
    switch (i)
    {
      case 0:
        str += 'A';
        break;
      default:
        str += 'B';
        continue;
    }

    str += 'C';
  }
}
assert (str === 'ACBBBBBBBBB');
