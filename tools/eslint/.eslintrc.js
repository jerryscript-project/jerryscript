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

var style = {
  "array-bracket-spacing": [2, "always", {
    "singleValue": true,
    "objectsInArrays": true,
    "arraysInArrays": true,
  }],
  "block-spacing": [2, "never"],
  "brace-style": [2, "1tbs"],
  "comma-dangle": [2, {
    "arrays": "always-multiline",
    "exports": "always-multiline",
    "functions": "never",
    "imports": "always-multiline",
    "objects": "always-multiline",
  }],
  "comma-spacing": 2,
  "comma-style": 2,
  "computed-property-spacing": 2,
  "eol-last": 2,
  "func-call-spacing": 2,
  "indent": ["error", 2],
  "key-spacing": 2,
  "keyword-spacing": 2,
  "linebreak-style": 0,
  "no-multi-spaces": 2,
  "no-multi-str": 2,
  "no-whitespace-before-property": 2,
  "no-multiple-empty-lines": [2, {max: 1}],
  "no-tabs": 2,
  "no-multi-str": 0,
  "no-trailing-spaces": 2,
  "quotes": [2, "single"],
  "semi": 2,
  "semi-spacing": 2,
  "space-before-blocks": 2,
  "space-before-function-paren": [2, {
    "anonymous": "never",
    "named": "never",
  }],
  "space-in-parens": 2,
  "spaced-comment": [2, "always"],
  "switch-colon-spacing": 2
}

module.exports = {
    "env": {
        "es2020": true
    },
    "parserOptions": {
        "ecmaVersion": 11,
    },
    "rules": style
};
