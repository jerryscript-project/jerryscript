# JerryScript Debugger

## Dependencies

- node.js
- `yarn` or `npm` (the steps below use `yarn`)

## Installing

```
$ cd jerryscript/jerry-debugger/jerry-client-ts
$ yarn install
```

## Using

TODO! We need to write the code first :D
We're aiming for a CLI that is the same as `node`'s so that any IDE that can interact with `node` to debug .js code, can interact with the JerryScript debugger.

## Building in watch mode

This will make the TypeScript compiler monitor the source files and re-build files whenever there is a change.

```
$ cd jerryscript/jerry-debugger/jerry-client-ts
$ yarn build:watch
```

## Running tests

```
$ cd jerryscript/jerry-debugger/jerry-client-ts
$ yarn test
```

## Running tests in watch mode

This will make the test runner monitor the source files and re-run the tests whenever there is a change.

```
$ cd jerryscript/jerry-debugger/jerry-client-ts
$ yarn test:watch
```

## Running the linter

```
$ cd jerryscript/jerry-debugger/jerry-client-ts
$ yarn lint
```

## TODO

- Hook up build + test + lint into travis.yml
- Fix "bin" files in package.json to get rid of jerry-debugger.sh
- Tons of work
- Set up publishing to npm
