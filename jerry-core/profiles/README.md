### About profile files

Specify compile definitions in profile files to use when compiling the `jerry-core` target.

The default profile is ``es.next`` which enables all of the currently implemented features.

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
# Disable not needed features
JERRY_BUILTIN_DATAVIEW=0
JERRY_BUILTIN_MAP=0
JERRY_BUILTIN_PROMISE=0
JERRY_BUILTIN_SET=0
JERRY_BUILTIN_TYPEDARRAY=0
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

#### ES2015+ features

* `JERRY_BUILTIN_BIGINT`:
  Enables or disables the [BigInt](https://262.ecma-international.org/11.0/#sec-ecmascript-language-types-bigint-type) syntax and built-in.
* `JERRY_BUILTIN_DATAVIEW`:
  Enables or disables the [DataView](https://www.ecma-international.org/ecma-262/6.0/#sec-dataview-objects) built-in.
* `JERRY_BUILTIN_GLOBAL_THIS`:
  Enables or disables the [GlobalThisValue](https://262.ecma-international.org/11.0/#sec-globalthis) built-in.
* `JERRY_BUILTIN_MAP`:
  Enables or disables the [Map](http://www.ecma-international.org/ecma-262/6.0/#sec-keyed-collection) built-ins.
* `JERRY_BUILTIN_PROMISE`:
  Enables or disables the [Promise](http://www.ecma-international.org/ecma-262/6.0/#sec-promise-objects) built-in.
* `JERRY_BUILTIN_PROXY`:
  Enables or disables the [Proxy](https://262.ecma-international.org/11.0/#sec-proxy-object-internal-methods-and-internal-slots) related internal workings and built-in.
* `JERRY_BUILTIN_REALMS`:
  Enables or disables the [Realms](https://262.ecma-international.org/11.0/#sec-code-realms) support in the engine.
* `JERRY_BUILTIN_REFLECT`:
  Enables or disables the [Reflext](https://262.ecma-international.org/11.0/#sec-reflect-object) built-in.
* `JERRY_BUILTIN_SET`:
  Enables or disables the [Set](https://www.ecma-international.org/ecma-262/6.0/#sec-set-objects) built-in.
* `JERRY_BUILTIN_TYPEDARRAY`:
  Enables or disables the [ArrayBuffer](http://www.ecma-international.org/ecma-262/6.0/#sec-arraybuffer-objects) and [TypedArray](http://www.ecma-international.org/ecma-262/6.0/#sec-typedarray-objects) built-ins.
* `JERRY_BUILTIN_WEAKMAP`:
  Enables or disables the [WeakMap](https://262.ecma-international.org/11.0/#sec-weakmap-objects) built-in.
* `JERRY_BUILTIN_WEAKSET`:
  Enables or disables the [WeakSet](https://262.ecma-international.org/11.0/#sec-weakmap-objects) built-in.
* `JERRY_BUILTIN_WEAKREF`:
  Enables or disables the [WeakRef](https://tc39.es/ecma262/#sec-weak-ref-constructor) built-in.
* `JERRY_MODULE_SYSTEM`:
  Enables or disable the [module system](http://www.ecma-international.org/ecma-262/6.0/#sec-modules) language element.
* `JERRY_ESNEXT`: Enables or disables all of the implemented [ECMAScript2015+ features](http://www.ecma-international.org/ecma-262/10.0/) above.
  * [arrow functions](http://www.ecma-international.org/ecma-262/6.0/#sec-arrow-function-definitions) language element.
  * [async functions](https://262.ecma-international.org/11.0/#sec-async-function-definitions) language element.
  * [class](https://www.ecma-international.org/ecma-262/6.0/#sec-class-definitions) language element.
  * [default value](http://www.ecma-international.org/ecma-262/6.0/#sec-function-definitions) for formal parameters.
  * [destructuring assignment](http://www.ecma-international.org/ecma-262/6.0/#sec-destructuring-assignment) language element.
  * [destructuring binding pattern](http://www.ecma-international.org/ecma-262/6.0/#sec-destructuring-binding-patterns) declarations.
  * [enhanced object initializer](http://www.ecma-international.org/ecma-262/6.0/#sec-object-initializer) language element.
  * [for-of](https://www.ecma-international.org/ecma-262/6.0/#sec-for-in-and-for-of-statements) language element.
  * [for-await-of](https://262.ecma-international.org/11.0/#sec-for-in-and-for-of-statements) language element.
  * [generator functions](http://www.ecma-international.org/ecma-262/6.0/#sec-generator-function-definitions) language element.
  * [iterator](https://www.ecma-international.org/ecma-262/6.0/#sec-iterator-interface) built-in.
  * [nullish coalescing](https://262.ecma-international.org/11.0/#prod-CoalesceExpression) built-in.
  * [numeric separator](https://github.com/tc39/proposal-numeric-separator) language element.
  * [rest parameter](http://www.ecma-international.org/ecma-262/6.0/#sec-function-definitions) language element.
  * [rest operator with object destructuring](https://262.ecma-international.org/11.0/#prod-ObjectBindingPattern) language element.
  * [spread](https://262.ecma-international.org/11.0/#prod-SpreadElement) syntax.
  * [symbol](https://www.ecma-international.org/ecma-262/6.0/#sec-symbol-objects) language element.
  * [template strings](http://www.ecma-international.org/ecma-262/6.0/#sec-static-semantics-templatestrings) language element.

  Furthermore all builtins follow the latest ECMAScript specification including those which behave differently in ES5.1.
  This option is evaulated first, any other `JERRY_<name>` defines will override that specific entry.
  Equivalent with setting the following defines to the `JERRY_ESNEXT` value:
    * `JERRY_BUILTIN_BIGINT`
    * `JERRY_BUILTIN_DATAVIEW`
    * `JERRY_BUILTIN_GLOBAL_THIS`
    * `JERRY_BUILTIN_MAP`
    * `JERRY_BUILTIN_PROMISE`
    * `JERRY_BUILTIN_PROXY`
    * `JERRY_BUILTIN_REALMS`
    * `JERRY_BUILTIN_REFLECT`
    * `JERRY_BUILTIN_SET`
    * `JERRY_BUILTIN_TYPEDARRAY`
    * `JERRY_BUILTIN_WEAKMAP`
    * `JERRY_BUILTIN_WEAKSET`
    * `JERRY_MODULE_SYSTEM`
