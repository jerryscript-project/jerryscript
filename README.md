# JerryScript: JavaScript engine for Internet of Things
[![Join the chat at https://gitter.im/Samsung/jerryscript](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/Samsung/jerryscript?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![License](https://img.shields.io/badge/licence-Apache%202.0-brightgreen.svg?style=flat)](LICENSE)


JerryScript is the lightweight JavaScript engine for very constrained devices such as microcontrollers:
- Only few kilobytes of RAM available to the engine (&lt;64 KB RAM)
- Constrained ROM space for the code of the engine (&lt;200 KB ROM)

Additional informantion can be found on our [project page](http://samsung.github.io/jerryscript/) and  [wiki](https://github.com/Samsung/jerryscript/wiki).

## Quick Start
### Getting Sources
```bash
git clone https://github.com/Samsung/jerryscript.git jr
cd jr
```

### Building
```bash
make release.linux -j
```

For Additional information see [Development](docs/DEVELOPMENT.md).

## Documentation
- [API Reference](docs/API-REFERENCE.md)
- [API Example](docs/API-EXAMPLE.md)

## License
JerryScript is Open Source software under the [Apache 2.0 license](https://www.apache.org/licenses/LICENSE-2.0). Complete license and copyright information can be found within the code.

> Copyright 2015 Samsung Electronics Co., Ltd.

> Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
