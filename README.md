![](https://github.com/jerryscript-project/jerryscript/blob/master/LOGO.png)
# JerryScript: JavaScript engine for the Internet of Things
[![License](https://img.shields.io/badge/licence-Apache%202.0-brightgreen.svg?style=flat)](LICENSE)
[![GitHub Actions Status](https://github.com/jerryscript-project/jerryscript/workflows/JerryScript%20CI/badge.svg)](https://github.com/jerryscript-project/jerryscript/actions)
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bhttps%3A%2F%2Fgithub.com%2Fjerryscript-project%2Fjerryscript.svg?type=shield)](https://app.fossa.io/projects/git%2Bhttps%3A%2F%2Fgithub.com%2Fjerryscript-project%2Fjerryscript?ref=badge_shield)
[![IRC Channel](https://img.shields.io/badge/chat-on%20freenode-brightgreen.svg)](https://kiwiirc.com/client/irc.freenode.net/#jerryscript)

JerryScript is a lightweight JavaScript engine for resource-constrained devices such as microcontrollers. It can run on devices with less than 64 KB of RAM and less than 200 KB of flash memory.

Key characteristics of JerryScript:
* Full ECMAScript 5.1 standard compliance
* ECMAScript 2025 standard compliance is 70%
  * [Kangax Compatibilty Table](https://compat-table.github.io/compat-table/es2016plus/)
* 258K binary size when compiled for ARM Thumb-2
* Heavily optimized for low memory consumption
* Written in C99 for maximum portability
* Snapshot support for precompiling JavaScript source code to byte code
* Mature C API, easy to embed in applications

Additional information can be found on our [project page](http://jerryscript.net) and [Wiki](https://github.com/jerryscript-project/jerryscript/wiki).

Mailing list: jerryscript-dev@groups.io, you can subscribe [here](https://groups.io/g/jerryscript-dev) and access the mailing list archive [here](https://groups.io/g/jerryscript-dev/topics).

## Quick Start
### Getting the sources
```bash
git clone https://github.com/jerryscript-project/jerryscript.git
cd jerryscript
```

### Building JerryScript
```bash
python tools/build.py
```

For additional information see [Getting Started](docs/00.GETTING-STARTED.md).

## Documentation
- [Getting Started](docs/00.GETTING-STARTED.md)
- [Configuration](docs/01.CONFIGURATION.md)
- [API Reference](docs/02.API-REFERENCE.md)
- [API Example](docs/03.API-EXAMPLE.md)
- [Port API](docs/05.PORT-API.md)
- [Reference Couting](docs/06.REFERENCE-COUNTING.md)
- [Debugger](docs/07.DEBUGGER.md)
- [Coding Standards](docs/08.CODING-STANDARDS.md)
- [Arguments Extension API](docs/09.EXT-REFERENCE-ARG.md)
- [Property Extension API](docs/10.EXT-REFERENCE-HANDLER.md)
- [Autorelease Extension API](docs/11.EXT-REFERENCE-AUTORELEASE.md)
- [Module Extension API](docs/12.EXT-REFERENCE-MODULE.md)
- [Debugger Transport Interface](docs/13.DEBUGGER-TRANSPORT.md)
- [Scope Extension API](docs/14.EXT-REFERENCE-HANDLE-SCOPE.md)
- [Module System](docs/15.MODULE-SYSTEM.md)
- [Migration Guide](docs/16.MIGRATION-GUIDE.md)

## Contributing
The project can only accept contributions which are licensed under the [Apache License 2.0](LICENSE) and are signed according to the JerryScript [Developer's Certificate of Origin](DCO.md). For further information please see our [Contribution Guidelines](CONTRIBUTING.md).

## License
JerryScript is open source software under the [Apache License 2.0](LICENSE). Complete license and copyright information can be found in the source code.

[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bhttps%3A%2F%2Fgithub.com%2Fjerryscript-project%2Fjerryscript.svg?type=large)](https://app.fossa.io/projects/git%2Bhttps%3A%2F%2Fgithub.com%2Fjerryscript-project%2Fjerryscript?ref=badge_large)

> Copyright JS Foundation and other contributors, http://js.foundation

> Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
