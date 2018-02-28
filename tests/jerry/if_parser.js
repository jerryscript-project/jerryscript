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

function test_parse_error (txt) {
  try {
    eval (txt)
    assert (false)
  } catch (e){
    assert (e instanceof SyntaxError)
  }
}

var if1=
"if (false)() print ('t')" +
"else print ('f')"
test_parse_error (if1)

test_parse_error ("if (true)() { print ('t') }")
test_parse_error ("if {} (true) print ('t')")
test_parse_error ("if (true false) print ('t')")
test_parse_error ("if (true && || false) print ('t')")
test_parse_error ("if (&& true) print ('t')")
test_parse_error ("if (true ||) print ('t')")
test_parse_error ("if (true && {false || true}) print ('t')")

var elseif1 =
"if (false) print ('if statement') " +
"elseif (false) print ('else if statement') " +
"else print ('else statement') "
test_parse_error (elseif1);

var elseif2 =
"if (false) print ('if statement') " +
"elif (false) print ('else if statement') " +
"else print ('else statement') "
test_parse_error (elseif2)

var elseif3 =
"if (false) print ('if statement') " +
"else (false) print ('else if statement') " +
"else print ('else statement') "
test_parse_error (elseif3)
