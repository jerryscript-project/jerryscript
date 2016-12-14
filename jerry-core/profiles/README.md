
### About profile files

Specify compile definitions in profile files to use when compiling the `jerry-core` target.

The default profile is ``es5.1`` which disables the ArrayBuffer built-in.

### Using profiles with the build system

You can specify the profile for the build system in the following ways:
  * with absolute path
  * with a name (this options selects profiles/$(name).profile file)

#### Restrictions
Only single line options are allowed in the profile file. Any line starting with hash-mark is ignored. Semicolon character is not allowed.

### Example usage:

#### 1. Using the build script

```
# assuming you are in jerryscript folder
./tools/build.py --profile=/absolute/path/to/my_profile.any_extension
```

or

```
# assuming you are in jerryscript folder
./tools/build.py --profile=minimal
```

This command selects the profile/minimal.profile file.

#### 2. Using only CMake build system

Set FEATURE_PROFILE option to one of the following values:
* the profile with absolute path
* name of the profile (which needs to exist in the `profiles` folder)
