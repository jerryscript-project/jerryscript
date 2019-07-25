![](https://github.com/jerryscript-project/jerryscript/blob/master/LOGO.png)
# JerryScript: JavaScript engine for the Internet of Things
[![License](https://img.shields.io/badge/licence-Apache%202.0-brightgreen.svg?style=flat)](LICENSE)
[![Travis CI Build Status](https://travis-ci.org/jerryscript-project/jerryscript.svg?branch=master)](https://travis-ci.org/jerryscript-project/jerryscript)
[![AppVeyor Build Status](https://ci.appveyor.com/api/projects/status/ct8reap35u2vooa5/branch/master?svg=true)](https://ci.appveyor.com/project/jerryscript-project/jerryscript/branch/master)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/12127/badge.svg)](https://scan.coverity.com/projects/jerryscript-project)
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bhttps%3A%2F%2Fgithub.com%2Fjerryscript-project%2Fjerryscript.svg?type=shield)](https://app.fossa.io/projects/git%2Bhttps%3A%2F%2Fgithub.com%2Fjerryscript-project%2Fjerryscript?ref=badge_shield)
[![SonarQube](https://sonarcloud.io/api/project_badges/measure?project=jerryscript-project_jerryscript&metric=ncloc)](https://sonarcloud.io/dashboard?id=jerryscript-project_jerryscript)
[![IRC Channel](https://img.shields.io/badge/chat-on%20freenode-brightgreen.svg)](https://kiwiirc.com/client/irc.freenode.net/#jerryscript)

JerryScript is a lightweight JavaScript engine for resource-constrained devices such as microcontrollers. It can run on devices with less than 64 KB of RAM and less than 200 KB of flash memory.

Key characteristics of JerryScript:
* Full ECMAScript 5.1 standard compliance
* 160K binary size when compiled for ARM Thumb-2
* Heavily optimized for low memory consumption
* Written in C99 for maximum portability
* Snapshot support for precompiling JavaScript source code to byte code
* Mature C API, easy to embed in applications

Additional information can be found on our [project page](http://jerryscript.net) and [Wiki](https://github.com/jerryscript-project/jerryscript/wiki).

Memory usage and Binary footprint are measured at [here](https://jerryscript-project.github.io/jerryscript-test-results) with real target daily.

The following table shows the latest results on the devices:

|  STM32F4-Discovery  | [![Remote Testrunner](https://firebasestorage.googleapis.com/v0/b/jsremote-testrunner.appspot.com/o/status%2Fjerryscript%2Fstm32f4dis.svg?alt=media&token=1)](https://jerryscript-project.github.io/jerryscript-test-results/?view=stm32f4dis) |
|        :---:        |                                             :---:                                                                                         |
|  **Raspberry Pi 2** | [![Remote Testrunner](https://firebasestorage.googleapis.com/v0/b/jsremote-testrunner.appspot.com/o/status%2Fjerryscript%2Frpi2.svg?alt=media&token=1)](https://jerryscript-project.github.io/jerryscript-test-results/?view=rpi2)       |

IRC channel: #jerryscript on [freenode](https://freenode.net)
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
- [Internals](docs/04.INTERNALS.md)
- [Migration Guide](docs/16.MIGRATION-GUIDE.md)

## Contributing
The project can only accept contributions which are licensed under the [Apache License 2.0](LICENSE) and are signed according to the JerryScript [Developer's Certificate of Origin](DCO.md). For further information please see our [Contribution Guidelines](CONTRIBUTING.md).

## License
JerryScript is open source software under the [Apache License 2.0](LICENSE). Complete license and copyright information can be found in the source code.

[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bhttps%3A%2F%2Fgithub.com%2Fjerryscript-project%2Fjerryscript.svg?type=large)](https://app.fossa.io/projects/git%2Bhttps%3A%2F%2Fgithub.com%2Fjerryscript-project%2Fjerryscript?ref=badge_large)

> Copyright JS Foundation and other contributors, http://js.foundation

> Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
