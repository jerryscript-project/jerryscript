### About profile files

Specify compile definitions in profile files to use when compiling the `jerry-core` target.

The default profile is ``es5.1`` which disables the ES2015 features.

### Using profiles with the build system

You can specify the profile for the build system in the following ways:
  * with absolute path
  * with a name (this options selects profiles/$(name).profile file)

#### Restrictions
Only single line options are allowed in the profile file. Any line starting with hash-mark is ignored. Semicolon character is not allowed.


### Example usage

#### 1. Using the build script

If you want to use a predefined profile, run the build script as follows
(assuming that you are in the project root folder):

```
./tools/build.py --profile=minimal
```

Alternatively, if you want to use a custom profile at
`/absolute/path/to/my.profile`:

```
# Turn off every ES2015 feature EXCEPT the arrow functions
CONFIG_DISABLE_ES2015_BUILTIN
CONFIG_DISABLE_ES2015_CLASS
CONFIG_DISABLE_ES2015_FUNCTION_PARAMETER_INITIALIZER
CONFIG_DISABLE_ES2015_MAP_BUILTIN
CONFIG_DISABLE_ES2015_OBJECT_INITIALIZER
CONFIG_DISABLE_ES2015_PROMISE_BUILTIN
CONFIG_DISABLE_ES2015_SYMBOL_BUILTIN
CONFIG_DISABLE_ES2015_TEMPLATE_STRINGS
CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN
```

Run the build script as follows (assuming that you are in the project root
folder):

```
./tools/build.py --profile=/absolute/path/to/my.profile
```


#### 2. Using only CMake build system

Set FEATURE_PROFILE option to one of the following values:
* the profile with absolute path
* name of the profile (which needs to exist in the `profiles` folder)


### Configurations

In JerryScript all of the features are enabled by default, so an empty profile file turns on all of the available ECMA features.

* `CONFIG_DISABLE_ANNEXB_BUILTIN`:
  Disable the [Annex B](http://www.ecma-international.org/ecma-262/5.1/index.html#sec-B) of the ECMA5.1 standard.
* `CONFIG_DISABLE_ARRAY_BUILTIN`:
  Disable the [Array](http://www.ecma-international.org/ecma-262/5.1/index.html#sec-15.4) built-in.
* `CONFIG_DISABLE_BOOLEAN_BUILTIN`:
  Disable the [Boolean](http://www.ecma-international.org/ecma-262/5.1/index.html#sec-15.6) built-in.
* `CONFIG_DISABLE_DATE_BUILTIN`:
  Disable the [Date](http://www.ecma-international.org/ecma-262/5.1/index.html#sec-15.9) built-in.
* `CONFIG_DISABLE_ERROR_BUILTINS`:
  Disable the [Native Error Types](http://www.ecma-international.org/ecma-262/5.1/index.html#sec-15.11.6) (EvalError, RangeError, ReferenceError, SyntaxError, TypeError, URIError).
  **Note**: The [Error](http://www.ecma-international.org/ecma-262/5.1/index.html#sec-15.11.2) object remains available.
* `CONFIG_DISABLE_JSON_BUILTIN`:
  Disable the [JSON](http://www.ecma-international.org/ecma-262/5.1/index.html#sec-15.12) built-in.
* `CONFIG_DISABLE_MATH_BUILTIN`:
  Disable the [Math](http://www.ecma-international.org/ecma-262/5.1/index.html#sec-15.8) built-in.
* `CONFIG_DISABLE_NUMBER_BUILTIN`:
  Disable the [Number](http://www.ecma-international.org/ecma-262/5.1/index.html#sec-15.7) built-in.
* `CONFIG_DISABLE_REGEXP_BUILTIN`:
  Disable the [RegExp](http://www.ecma-international.org/ecma-262/5.1/index.html#sec-15.10) built-in.
* `CONFIG_DISABLE_STRING_BUILTIN`:
  Disable the [String](http://www.ecma-international.org/ecma-262/5.1/index.html#sec-15.5) built-in.
* `CONFIG_DISABLE_BUILTINS`:
  Disable all of the [Standard Built-in ECMAScript 5.1 Objects](http://www.ecma-international.org/ecma-262/5.1/index.html#sec-15)
  (equivalent to `CONFIG_DISABLE_ANNEXB_BUILTIN`, `CONFIG_DISABLE_ARRAY_BUILTIN`, `CONFIG_DISABLE_BOOLEAN_BUILTIN`, `CONFIG_DISABLE_DATE_BUILTIN`, `CONFIG_DISABLE_ERROR_BUILTINS`, `CONFIG_DISABLE_JSON_BUILTIN`, `CONFIG_DISABLE_MATH_BUILTIN`, `CONFIG_DISABLE_NUMBER_BUILTIN`, `CONFIG_DISABLE_REGEXP_BUILTIN`, and `CONFIG_DISABLE_STRING_BUILTIN`).

* `CONFIG_DISABLE_ES2015_ARROW_FUNCTION`:
  Disable the [arrow functions](http://www.ecma-international.org/ecma-262/6.0/#sec-arrow-function-definitions).
* `CONFIG_DISABLE_ES2015_BUILTIN`:
  Disable the built-in updates of the 5.1 standard. There are some differences in those built-ins which available in both [5.1](http://www.ecma-international.org/ecma-262/5.1/) and [2015](http://www.ecma-international.org/ecma-262/6.0/) versions of the standard. JerryScript uses the latest definition by default.
* `CONFIG_DISABLE_ES2015_CLASS`:
  Disable the [class](https://www.ecma-international.org/ecma-262/6.0/#sec-class-definitions) language element.
* `CONFIG_DISABLE_ES2015_FUNCTION_PARAMETER_INITIALIZER`:
  Disable the [default value](http://www.ecma-international.org/ecma-262/6.0/#sec-function-definitions) for formal parameters.
* `CONFIG_DISABLE_ES2015_MAP_BUILTIN`:
  Disable the [Map](http://www.ecma-international.org/ecma-262/6.0/#sec-keyed-collection) built-ins.
* `CONFIG_DISABLE_ES2015_OBJECT_INITIALIZER`:
  Disable the [enhanced object initializer](http://www.ecma-international.org/ecma-262/6.0/#sec-object-initializer) language element.
* `CONFIG_DISABLE_ES2015_SYMBOL_BUILTIN`:
  Disable the [Symbol](https://www.ecma-international.org/ecma-262/6.0/#sec-symbol-objects) built-in.
* `CONFIG_DISABLE_ES2015_PROMISE_BUILTIN`:
  Disable the [Promise](http://www.ecma-international.org/ecma-262/6.0/#sec-promise-objects) built-in.
* `CONFIG_DISABLE_ES2015_TEMPLATE_STRINGS`:
  Disable the [template strings](http://www.ecma-international.org/ecma-262/6.0/#sec-static-semantics-templatestrings).
* `CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN`:
  Disable the [ArrayBuffer](http://www.ecma-international.org/ecma-262/6.0/#sec-arraybuffer-objects) and [TypedArray](http://www.ecma-international.org/ecma-262/6.0/#sec-typedarray-objects) built-ins.
* `CONFIG_DISABLE_ES2015`: Disable all of the implemented [ECMAScript2015 features](http://www.ecma-international.org/ecma-262/6.0/).
  (equivalent to `CONFIG_DISABLE_ES2015_ARROW_FUNCTION`, `CONFIG_DISABLE_ES2015_BUILTIN`, `CONFIG_DISABLE_ES2015_CLASS`,
  `CONFIG_DISABLE_ES2015_FUNCTION_PARAMETER_INITIALIZER`, `CONFIG_DISABLE_ES2015_MAP_BUILTIN`, `CONFIG_DISABLE_ES2015_OBJECT_INITIALIZER`,
  `CONFIG_DISABLE_ES2015_SYMBOL_BUILTIN`, `CONFIG_DISABLE_ES2015_PROMISE_BUILTIN`, `CONFIG_DISABLE_ES2015_TEMPLATE_STRINGS`, and `CONFIG_DISABLE_ES2015_TYPEDARRAY_BUILTIN`).
