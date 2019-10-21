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
# Turn off every ES2015 feature EXCEPT the module system
JERRY_ES2015_BUILTIN=0
JERRY_ES2015_BUILTIN_DATAVIEW=0
JERRY_ES2015_BUILTIN_ITERATOR=0
JERRY_ES2015_BUILTIN_MAP=0
JERRY_ES2015_BUILTIN_PROMISE=0
JERRY_ES2015_BUILTIN_SET=0
JERRY_ES2015_BUILTIN_TYPEDARRAY=0
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
The defines can have a value of `0` or `1`. If for whatever reason one of them are not defined, it is treated as if it were
defined to `1`.

#### ES 5.1 features

* `JERRY_BUILTIN_ANNEXB`:
  Enables or disables the [Annex B](http://www.ecma-international.org/ecma-262/5.1/index.html#sec-B) of the ECMA5.1 standard.
* `JERRY_BUILTIN_ARRAY`:
  Enables or disable the [Array](http://www.ecma-international.org/ecma-262/5.1/index.html#sec-15.4) built-in.
* `JERRY_BUILTIN_BOOLEAN`:
  Enables or disables the [Boolean](http://www.ecma-international.org/ecma-262/5.1/index.html#sec-15.6) built-in.
* `JERRY_BUILTIN_DATE`:
  Enables or disables the [Date](http://www.ecma-international.org/ecma-262/5.1/index.html#sec-15.9) built-in.
* `JERRY_BUILTIN_ERRORS`:
  Enables or disables the [Native Error Types](http://www.ecma-international.org/ecma-262/5.1/index.html#sec-15.11.6) (EvalError, RangeError, ReferenceError, SyntaxError, TypeError, URIError).
  **Note**: The [Error](http://www.ecma-international.org/ecma-262/5.1/index.html#sec-15.11.2) object remains available.
* `JERRY_BUILTIN_JSON`:
  Enables or disables the [JSON](http://www.ecma-international.org/ecma-262/5.1/index.html#sec-15.12) built-in.
* `JERRY_BUILTIN_MATH`:
  Enables or disables the [Math](http://www.ecma-international.org/ecma-262/5.1/index.html#sec-15.8) built-in.
* `JERRY_BUILTIN_NUMBER`:
  Enables or disables the [Number](http://www.ecma-international.org/ecma-262/5.1/index.html#sec-15.7) built-in.
* `JERRY_BUILTIN_REGEXP`:
  Enables or disables the [RegExp](http://www.ecma-international.org/ecma-262/5.1/index.html#sec-15.10) built-in.
* `JERRY_BUILTIN_STRING`:
  Enables or disables the [String](http://www.ecma-international.org/ecma-262/5.1/index.html#sec-15.5) built-in.
* `JERRY_BUILTINS`:
  Enables or disable all of the [Standard Built-in ECMAScript 5.1 Objects](http://www.ecma-international.org/ecma-262/5.1/index.html#sec-15)
  This option is evaulated first, any other `JERRY_BUILTIN_<name>` defines will override that specific builtin config.
  Equivalent with setting the following defines to the `JERRY_BUILTINS` value:
    * `JERRY_BUILTIN_ANNEXB`
    * `JERRY_BUILTIN_ARRAY`
    * `JERRY_BUILTIN_BOOLEAN`
    * `JERRY_BUILTIN_DATE`
    * `JERRY_BUILTIN_ERRORS`
    * `JERRY_BUILTIN_JSON`
    * `JERRY_BUILTIN_MATH`
    * `JERRY_BUILTIN_NUMBER`
    * `JERRY_BUILTIN_REGEXP`
    * `JERRY_BUILTIN_STRING`

#### ES6 (ES 2015) features

* `JERRY_ES2015_BUILTIN`:
  Enables or disables the built-in updates of the 5.1 standard. There are some differences in those built-ins which available in both [5.1](http://www.ecma-international.org/ecma-262/5.1/) and [2015](http://www.ecma-international.org/ecma-262/6.0/) versions of the standard. JerryScript uses the latest definition by default.
* `JERRY_ES2015_BUILTIN_DATAVIEW`:
  Enables or disables the [DataView](https://www.ecma-international.org/ecma-262/6.0/#sec-dataview-objects) built-in.
* `JERRY_ES2015_BUILTIN_ITERATOR`:
  Enables or disables the [Iterator](https://www.ecma-international.org/ecma-262/6.0/#sec-iterator-interface) built-in.
* `JERRY_ES2015_BUILTIN_MAP`:
  Enables or disables the [Map](http://www.ecma-international.org/ecma-262/6.0/#sec-keyed-collection) built-ins.
* `JERRY_ES2015_BUILTIN_SET`:
  Enables or disables the [SET](https://www.ecma-international.org/ecma-262/6.0/#sec-set-objects) built-in.
* `JERRY_ES2015_BUILTIN_PROMISE`:
  Enables or disables the [Promise](http://www.ecma-international.org/ecma-262/6.0/#sec-promise-objects) built-in.
* `JERRY_ES2015_BUILTIN_TYPEDARRAY`:
  Enables or disables the [ArrayBuffer](http://www.ecma-international.org/ecma-262/6.0/#sec-arraybuffer-objects) and [TypedArray](http://www.ecma-international.org/ecma-262/6.0/#sec-typedarray-objects) built-ins.
* `JERRY_ES2015_MODULE_SYSTEM`:
  Enables or disable the [module system](http://www.ecma-international.org/ecma-262/6.0/#sec-modules) language element.
* `JERRY_ES2015`: Enables or disable all of the implemented [ECMAScript2015 features](http://www.ecma-international.org/ecma-262/6.0/).
  * [arrow functions](http://www.ecma-international.org/ecma-262/6.0/#sec-arrow-function-definitions) language element.
  * [symbol](https://www.ecma-international.org/ecma-262/6.0/#sec-symbol-objects) language element.
  * [class](https://www.ecma-international.org/ecma-262/6.0/#sec-class-definitions) language element.
  * [for of](https://www.ecma-international.org/ecma-262/6.0/#sec-for-in-and-for-of-statements) language element.
  * [default value](http://www.ecma-international.org/ecma-262/6.0/#sec-function-definitions) for formal parameters.
  * [rest parameter](http://www.ecma-international.org/ecma-262/6.0/#sec-function-definitions) language element.
  * [enhanced object initializer](http://www.ecma-international.org/ecma-262/6.0/#sec-object-initializer) language element.
  * [template strings](http://www.ecma-international.org/ecma-262/6.0/#sec-static-semantics-templatestrings) language element.
  This option is evaulated first, any other `JERRY_ES2015_<name>` defines will override that specific entry.
  Equivalent with setting the following defines to the `JERRY_ES2015` value:
    * `JERRY_ES2015_BUILTIN`
    * `JERRY_ES2015_BUILTIN_DATAVIEW`
    * `JERRY_ES2015_BUILTIN_ITERATOR`
    * `JERRY_ES2015_BUILTIN_MAP`
    * `JERRY_ES2015_BUILTIN_SET`
    * `JERRY_ES2015_BUILTIN_PROMISE`
    * `JERRY_ES2015_BUILTIN_TYPEDARRAY`
    * `JERRY_ES2015_MODULE_SYSTEM`

