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
print ( JSON . stringify ( ctl_string ) == '"asd\\u001fasd"' )
escpad_string = "\"asda\sd"
print ( JSON . stringify ( escpad_string ) == '"\\"asdasd"' )
print ( JSON . stringify ( true ) === 'true' )
print ( JSON . stringify ( "foo" ) === '"foo"' )
print ( JSON . stringify ( null ) === 'null' )
print ( JSON . stringify ( RegExp ) === undefined )
print ( JSON . stringify ( new Number ( 1 ) ) === '1' )
print ( JSON . stringify ( new Boolean ( true ) ) === 'true' )
print ( JSON . stringify ( new String ( "foo" ) ) === '"foo"' )
empty_object = { }
print ( JSON . stringify ( empty_object ) == '{}' )
empty_object = { }
empty_object . $ = undefined
print ( JSON . stringify ( empty_object ) == '{}' )
p_object = { "a" : 1 , "b" : true , "c" : "foo" , "d" : null , "e" : undefined }
print ( JSON . stringify ( p_object ) == '{"a":1,"b":true,"c":"foo","d":null}' )
o_object = { "a" : new Number ( 1 ) , "b" : new Boolean ( true ) , "c" : new String ( "foo" ) }
print ( JSON . stringify ( o_object ) == '{"a":1,"b":true,"c":"foo"}' )
child = { "a" : 1 , "b" : new String ( "\nfoo" ) , $ : undefined }
parent = { "a" : true , "b" : child , "c" : null }
print ( JSON . stringify ( parent ) == '{"a":true,"b":{"a":1,"b":"\\nfoo"},"c":null}' )
recursive_object = { }
recursive_object . $ = 0
recursive_object . b = recursive_object
try { JSON . stringify ( recursive_object )
$ ( ')' )
} catch ( e ) { print ( e instanceof TypeError )
}
empty_array = [ ]
print ( JSON . stringify ( empty_array ) == '[]' )
array = [ undefined ]
print ( JSON . stringify ( array ) == '[null]' )
p_array = [ 1 , true , "foo" , null , undefined ]
print ( JSON . stringify ( p_array ) == '[1,true,"foo",null,null]' )
o_array = [ new Number ( 1 ) , new Boolean ( true ) , new String ( "foo" ) ]
print ( JSON . stringify ( o_array ) == '[1,true,"foo"]' )
child = [ 1 , new String ( "\nfoo" ) , undefined ]
parent = [ true , child , null ]
print ( JSON . stringify ( parent ) == '[true,[1,"\\nfoo",null],null]' )
recursive_array = [ ]
recursive_array [ 0 ] = 2
recursive_array [ 1 ] = recursive_array
try { JSON . stringify ( recursive_array )
$ ( $ )
} catch ( e ) { print ( e instanceof TypeError )
}
object = { "a" : 1 , "b" : [ 1 , true , { "a" : "foo" } ] }
print ( JSON . stringify ( object ) == '{"a":1,"b":[1,true,{"a":"foo"}]}' )
object = { "a" : [ 1 ] , "b" : { } }
print ( JSON . stringify ( object ) === '{"a":[1],"b":{}}' )
array = [ 1 , { "a" : 2 , "b" : true , c : [ 3 ] } ]
print ( JSON . stringify ( array ) == '[1,{"a":2,"b":true,"c":[3]}]' )
to_json_object = { }
to_json_object . $ = 0
to_json_object . $ = function ( ) {
}
var v1 = ( new Int8Array ( 149 ) ) . subarray ( 78 )
function replacer_function ( $ , value ) { if ( typeof ( value ) == "string" ) return "FOO"
return value
} object = { "a" : "JSON" , "b" : new String ( "JSON" ) , "c" : 3 }
print ( JSON . stringify ( object , replacer_function ) == '{"a":"FOO","b":"JSON","c":3}' )
filter = [ "a" , "b" ]
print ( JSON . stringify ( object , filter ) == '{"a":"JSON","b":"JSON"}' )
print ( JSON . stringify ( [ ] , [ "" , 4 ] ) === '[]' )
number = new Number ( 2.2 )
number . toString = { }
new Promise ( isFinite . toString ) . catch ( Promise . prototype . then )
try { } catch ( $ ) { $ ( $ instanceof $ )
}
function replacer_thrower ( ) { throw new ReferenceError ( "foo" )
} try { JSON . stringify ( object , replacer_thrower )
} catch ( e ) { print ( e . message === "foo" )
print ( e instanceof ReferenceError )
}
object = { "a" : 2 }
print ( JSON . stringify ( object , 3 ) == '{"a":2}' )
print ( JSON . stringify ( object , 0 ) == '{"a":2}' )
print ( JSON . stringify ( object , 0 ) == '{"a":2}' )
print ( JSON . stringify ( object , undefined ) == '{"a":2}' )
print ( JSON . stringify ( object , 0 ) == '{"a":2}' )
print ( JSON . stringify ( object , new Boolean ( 0 ) ) == '{"a":2}' )
print ( JSON . stringify ( object , 0 , "asd\u20400123456789" ) == '{\nasd⁀012345"a": 2\n}' )
print ( JSON . stringify ( object , 0 , 100 ) == '{\n          "a": 2\n}' )
print ( JSON . stringify ( object , 0 , - 5 ) == '{"a":2}' )
array = [ 2 ]
print ( JSON . stringify ( array , 0 , "   " ) == '[\n   2\n]' )
print ( JSON . stringify ( array , 0 , "asd" ) == '[\nasd2\n]' )
print ( JSON . stringify ( array , 0 , "asd0123456789" ) == '[\nasd01234562\n]' )
print ( JSON . stringify ( array , 0 , "asd\u20400123456789" ) == '[\nasd⁀0123452\n]' )
print ( JSON . stringify ( array , 0 , 100 ) == '[\n          2\n]' )
print ( JSON . stringify ( array , 0 , - 5 ) == '[2]' )
nested_object = { do : 2 , let : this }
print ( JSON . stringify ( nested_object , 0 , 2 ) == '{\n  "a": 2,\n  "b": {\n    "c": 1,\n    "d": true\n  }\n}' )
$ = [ $ , [ $ ] ]
$ ( $ . $ ( nested_array , $ , 2 ) == '[\n  2,\n  [\n    1,\n    true\n  ]\n]' )
$
$
$
$ ( $ . $ ( $ , 0 , $ ) == $ )
eval ( "{}/a/g" )
$ ( $ . $ ( $ , $ , [ $ , $ , 0 ] ) == $ )
$ ( $ . $ ( $ , $ , { $ : 3 } ) == $ )
