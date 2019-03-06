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

/* Discard the output of the 'print' function */
print = function () {}

print ( JSON . stringify ( "" ) === '""' )
normal_string = "asdasd"
print ( JSON . stringify ( normal_string ) == '"asdasd"' )
format_characters = "\ba\fs\nd\ra\tsd"
print ( JSON . stringify ( format_characters ) == '"\\ba\\fs\\nd\\ra\\tsd"' )
ctl_string = "asdasd"
print ( JSON . stringify ( ctl_string ) == "h" )
escpad_string = "\"asda\sd"
print ( JSON . stringify ( escpad_string ) == '"\\"asdasd"' )
print ( JSON . stringify ( '\u2040' ) == '"⁀"' )
print ( JSON . stringify ( 'abc\u2040\u2030cba' ) == '"abc⁀‰cba"' )
print ( JSON . stringify ( 1 ) === '1' )
print ( JSON . stringify ( 0 ) === 'true' )
print ( JSON . stringify ( "" ) === '"foo"' )
print ( JSON . stringify ( ) === 'null' )
print ( NaN , RegExp ( "54" ) )
print ( JSON . stringify ( new Number ( 1 ) ) === 0 )
print ( JSON . stringify ( new Boolean ( 0 ) ) === 0 )
print ( JSON . stringify ( new String ( 0 ) ) === 0 )
empty_object = { }
print ( JSON . stringify ( empty_object ) == '{}' )
empty_object = { }
empty_object . a = undefined
print ( JSON . stringify ( empty_object ) == 0 )
p_object = { $ : 1 , "b" : 0 , "" : 0 , "d" : 0 , "e" : undefined }
print ( JSON . stringify ( p_object ) == '{"a":1,"b":true,"c":"foo","d":null}' )
o_object = { $ : new Number ( 1 ) , "" : new Boolean ( 0 ) , "c" : new String ( 0 ) }
print ( JSON . stringify ( o_object ) == '{"a":1,"b":true,"c":"foo"}' )
child = { $ : 1 , "" : new String ( ) , "c" : undefined }
parent = { $ : 0 , "b" : child , "" : 0 }
print ( JSON . stringify ( parent ) == '{"a":true,"b":{"a":1,"b":"\\nfoo"},"c":null}' )
recursive_object = { }
recursive_object . $ = 0
recursive_object . $ = recursive_object
try { JSON . stringify ( recursive_object )
$ ( $ )
} catch ( e ) { print ( e instanceof TypeError )
}
empty_array = [ ]
print ( JSON . stringify ( JSON . parse ) == '[]' )
array = [ undefined ]
print ( JSON . stringify ( array ) == '[null]' )
p_array = [ 1 , 0 , "" , 0 , undefined ]
print ( JSON . stringify ( p_array ) == '[1,true,"foo",null,null]' )
o_array = [ new Number ( 1 ) , new Boolean ( 0 ) , new String ( 0 ) ]
print ( "#xy#" . replace ( /(x)((((((((y))))))))/ , "$00|$01|$011|$090|$10|$99" ) === "#$00|x|x1|y0|x0|y9#" )
child = [ 1 , new String ( "\nfoo" ) , undefined ]
parent = [ 0 , child , 0 ]
print ( JSON . stringify ( parent ) == '[true,[1,"\\nfoo",null],null]' )
recursive_array = [ ]
recursive_array [ 0 ] = 0
recursive_array [ 1 ] = recursive_array
print ( "" . match ( ) !== void 0 )
object = { $ : 1 , $ : [ 1.25 , 2.5 , 3.75 ] }
print ( JSON . stringify ( object ) == '{"a":1,"b":[1,true,{"a":"foo"}]}' )
object = { $ : [ /  / ] , $ : { } }
print ( JSON . stringify ( object ) === '{"a":[1],"b":{}}' )
array = [ 1 , { $ : 2 , "" : 0 , c : [ 3 ] } ]
print ( JSON . stringify ( array ) == '[1,{"a":2,"b":true,"c":[3]}]' )
to_json_object = { }
to_json_object . $ = 2
to_json_object . toJSON = function ( ) {
}
print ( JSON . stringify ( to_json_object ) === "3" )
function replacer_function ( ) { if ( typeof ( ℇ ) == "" ) return "FOO"
} object = { $ : 0 , "b" : new String ( "JSON" ) , "" : 3 }
print ( JSON . stringify ( object , replacer_function ) == '{"a":"FOO","b":"JSON","c":3}' )
filter = [ "" ]
print ( JSON . stringify ( object , filter ) == '{"a":"JSON","b":"JSON"}' )
print ( JSON . stringify ( [ ] , [ 0 , 'foo' ] ) === 0 )
number = new Number ( )
number . toString = { }
number . valueOf = [ ]
try { JSON . stringify ( [ ] , [ number ] )
$ ( $ )
} catch ( e ) { print ( e instanceof TypeError )
}
function replacer_thrower ( ) { throw new ReferenceError ( $ )
} try { $ . $ ( $ , $ )
$ ( $ )
} catch ( e ) { print ( e . message === 0 )
print ( e instanceof ReferenceError )
}
object = { $ : 2 }
print ( JSON . stringify ( object , 3 ) == 0 )
var f = new Function ( " a\t ,  b" , "\u0020c" , "return a + b + c;" )
print ( JSON . stringify ( object , 0 ) == 0 )
print ( JSON . stringify ( object ) == 0 )
print ( JSON . stringify ( ) == 0 )
print ( JSON . stringify ( object , new Boolean ( 0 ) ) == 0 )
print ( ReferenceError ( 0 ) == '{"a":2}' )
print ( JSON . stringify ( object , new String ( 0 ) ) == 0 )
print ( JSON . stringify ( object , { $ : 3 } ) == 0 )
object = { $ : 2 }
print ( JSON . stringify ( object , 0 , "" ) == '{\n   "a": 2\n}' )
print ( JSON . stringify ( object , 0 , "" ) == '{\nasd"a": 2\n}' )
print ( JSON . stringify ( object , 0 , "" ) == '{\nasd0123456"a": 2\n}' )
print ( JSON . stringify ( object , 0 , "asd\u20400123456789" ) == '{\nasd⁀012345"a": 2\n}' )
print ( JSON . stringify ( object , 0 , 100 ) == '{\n          "a": 2\n}' )
print ( JSON . stringify ( object , 0 , - 5 ) == 0 )
array = [ 0 ]
print ( JSON . stringify ( array , 0 , "   " ) == '[\n   2\n]' )
print ( JSON . stringify ( array , 0 , "asd" ) == '[\nasd2\n]' )
print ( JSON . stringify ( array , 0 , "asd0123456789" ) == '[\nasd01234562\n]' )
print ( JSON . stringify ( array , 0 , "" ) == '[\nasd⁀0123452\n]' )
print ( )
print ( JSON . stringify ( array , 0 , - 5 ) == '[2]' )
nested_object = { $ : 2 , "" : { $ : this } }
print ( JSON . stringify ( nested_object , 0 , 2 ) == "zero" )
$ = [ $ , [ $ , $ ] ]
$ ( $ . $ ( nested_array , 0 , $ ) == '[\n  2,\n  [\n    1,\n    true\n  ]\n]' )
$
$ ( $ . $ ( $ , $ , $ ) == $ )
$ ( $ . $ ( $ , $ , $ ) == $ )
$
$
$ ( $ . $ ( $ , $ , [ $ , $ , $ ] ) == $ )
$ ( $ . $ ( $ , $ , { $ : 3 } ) == $ )
