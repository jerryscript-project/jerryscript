![](https://github.com/Samsung/jerryscript/blob/master/LOGO.png)
# JerryScript: JavaScript engine for the Internet of Things
[![License](https://img.shields.io/badge/licence-Apache%202.0-brightgreen.svg?style=flat)](LICENSE)
[![Build Status](https://travis-ci.org/Samsung/jerryscript.svg?branch=master)](https://travis-ci.org/Samsung/jerryscript)

JerryScript is a lightweight JavaScript engine for resource-constrained devices such as microcontrollers. It can run on devices with less than 64 KB of RAM and less than 200 KB of flash memory.

Key characteristics of JerryScript:
* Full ECMAScript 5.1 standard compliance
* 160K binary size when compiled for ARM Thumb-2
* Heavily optimized for low memory consumption
* Written in C99 for maximum portability
* Snapshot support for precompiling JavaScript source code to byte code
* Mature C API, easy to embed in applications

Additional information can be found on our [project page](http://jerryscript.net) and [Wiki](https://github.com/Samsung/jerryscript/wiki).

IRC channel: #jerryscript on [freenode](https://freenode.net)  
Mailing list: jerryscript-dev@gna.org, you can subscribe [here](https://mail.gna.org/listinfo/jerryscript-dev) and access the mailing list archive [here](https://mail.gna.org/public/jerryscript-dev).

## Quick Start
### Getting the sources
```bash
git clone https://github.com/Samsung/jerryscript.git
cd jerryscript
```

### Building JerryScript
```bash
python tools/build.py
```

For additional information see [Getting Started](docs/01.GETTING-STARTED.md).

## Documentation
- [Getting Started](docs/01.GETTING-STARTED.md)
- [API Reference](docs/02.API-REFERENCE.md)
- [API Example](docs/03.API-EXAMPLE.md)
- [Internals](docs/04.INTERNALS.md)

## Contributing
The project can only accept contributions which are licensed under the [Apache License 2.0](LICENSE) and are signed according to the JerryScript [Developer's Certificate of Origin](DCO.md). For further information please see our [Contribution Guidelines](CONTRIBUTING.md).

## License
JerryScript is Open Source software under the [Apache License 2.0](LICENSE). Complete license and copyright information can be found in the source code.

> Copyright 2015 Samsung Electronics Co., Ltd.

> Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
