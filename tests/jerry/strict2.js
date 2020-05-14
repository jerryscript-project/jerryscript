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

function check_syntax_error (script)
{
  try
  {
    eval (script);
    assert (false);
  }
  catch (e)
  {
    assert (e instanceof SyntaxError);
  }
}

var
  implements = 0,
  private = 1,
  public = 2,
  interface = 3,
  package = 4,
  protected = 5,
  let = 6,
  yield = 7,
  static = 8;

check_syntax_error("'use strict'\nimplements")
check_syntax_error("'use strict'\n\\u0069mplements")
assert(eval("'use stric'\nimplements") === 0)
assert(eval("'use stric'\n\\u0069mplements") === 0)

check_syntax_error("'use strict'\nprivate")
check_syntax_error("'use strict'\n\\u0070rivate")
assert(eval("'use stric'\nprivate") === 1)
assert(eval("'use stric'\n\\u0070rivate") === 1)

check_syntax_error("'use strict'\npublic")
check_syntax_error("'use strict'\n\\u0070ublic")
assert(eval("'use stric'\npublic") === 2)
assert(eval("'use stric'\n\\u0070ublic") === 2)

check_syntax_error("'use strict'\ninterface")
check_syntax_error("'use strict'\n\\u0069nterface")
assert(eval("'use stric'\ninterface") === 3)
assert(eval("'use stric'\n\\u0069nterface") === 3)

check_syntax_error("'use strict'\npackage")
check_syntax_error("'use strict'\n\\u0070ackage")
assert(eval("'use stric'\npackage") === 4)
assert(eval("'use stric'\n\\u0070ackage") === 4)

check_syntax_error("'use strict'\nprotected")
check_syntax_error("'use strict'\n\\u0070rotected")
assert(eval("'use stric'\nprotected") === 5)
assert(eval("'use stric'\n\\u0070rotected") === 5)

check_syntax_error("'use strict'\nlet")
check_syntax_error("'use strict'\n\\u006cet")
assert(eval("'use stric'\nlet") === 6)
assert(eval("'use stric'\n\\u006cet") === 6)

check_syntax_error("'use strict'\nyield")
check_syntax_error("'use strict'\n\\u0079ield")
assert(eval("'use stric'\nyield") === 7)
assert(eval("'use stric'\n\\u0079ield") === 7)

check_syntax_error("'use strict'\nstatic")
check_syntax_error("'use strict'\n\\u0073tatic")
assert(eval("'use stric'\nstatic") === 8)
assert(eval("'use stric'\n\\u0073tatic") === 8)
