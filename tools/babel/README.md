# Converting incompatible features with Babel
To run ES6 sources with JerryScript that use unsupported language features, you can use Babel to transpile your code, which will output a semantically equivalent source file, where the unsupported features are replaced with ES5.1 code.
Babel is a JavaScript compiler that is used to convert ES2015+ code into a backward-compatible version. You can find more information [here](https://babeljs.io/).

## Example

```javascript
//Before
const fn = () => 1;

//After conversion

var fn = function fn() {
  return 1;
};
```
## Table of Contents
* **[Getting ready](#getting-ready)**
  * Installing node.js and npm
* **[Using babel](#using-babel)**
* **[Missing features/Polyfill](#missing-features)**

## Getting ready [](#getting-ready)

### 1. **Node.js and npm**

Start by updating the packages with

`$ sudo apt update`

Install `nodejs` using the apt package manager

`$ sudo apt install nodejs`

Check the version of **node.js** to verify that it installed

```bash
$ nodejs --version
Output: v8.10.0
```

Next up is installing **npm** with the following command

`$ sudo apt install npm`

Verify installation by typing:

```bash
$ npm --version
Output: 6.10.2
```

### 2. Using babel [](#using-babel)

Assuming you're in the tools/babel folder,

`$ sudo npm install`

After installing the dependencies it is ready to use.

Place the files/directories you want transpiled to the babel folder and run

`$ npm run transpile [name of input file/directory] [(OPTIONAL)name of output file/directory]`

If you want to use the same name, then only give the name once.

#### Missing features/Polyfill [](#missing-features)
Some features aren't implemented yet and found no babel plug-in for them.

* [Array.from](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array/from)
* [Array.prototype.fill](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Array/fill)
* [String.prototype.codePointAt](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String/codePointAt)

In the case you encounter those, use the polyfill found on the linked sites. Also, **be aware** there could be more.