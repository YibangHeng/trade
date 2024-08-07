# Welcome to trade

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![GitHub Workflow Status](https://img.shields.io/github/actions/workflow/status/YibangHeng/trade/ci.yml?branch=main)](https://github.com/YibangHeng/trade/actions/workflows/ci.yml)
[![PyPI Release](https://img.shields.io/pypi/v/trade.svg)](https://pypi.org/project/trade)

## Prerequisites

Building trade requires the following software installed:

* A C++20-compliant compiler

* [CMake](https://cmake.org) `>= 3.9`

* [vcpkg](https://vcpkg.io/) for third-party library management

* [Qt 6](https://www.qt.io/) for GUI application

* [Doxygen](https://www.doxygen.org) (optional, documentation building is skipped if missing)

* [Python](https://www.python.org) `>= 3.8` for building Python bindings

## Building trade

Make sure your setting the vcpkg and Qt environment variable correctly for CMake
before building.

For vcpkg, you need `CMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake`.

For Qt, you need `CMAKE_PREFIX_PATH=/path/to/qt/lib/cmake`.

The following sequence of commands builds trade.

It assumes that your current working directory is the top-level directory of the
freshly cloned repository:

```shell
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

The build process can be customized with the following CMake variables, which
can be set by adding `-D<var>={ON, OFF}` to the `cmake` call:

* `BUILD_TESTING`: Enable building of the test suite (default: `ON`)

* `BUILD_DOCS`: Enable building the documentation (default: `ON`)

* `BUILD_PYTHON`: Enable building the Python bindings (default: `ON`)

If you wish to build and install the project as a Python project without having
access to C++ build artifacts like libraries and executables, you can do so
using `pip` from the root directory:

```shell
python -m pip install .
```

## Testing trade

When built according to the above explanation (with `-DBUILD_TESTING=ON`), the
C++ test suite of `trade` can be run using `ctest` from the build directory:

```shell
cd build
ctest
```

The Python test suite can be run by first `pip`-installing the Python package
and then running `pytest` from the top-level directory:

```shell
python -m pip install .
pytest
```

## Documentation

trade provides a Doxygen documentation. You can build the documentation locally
by making sure that `Doxygen` is installed on your system and running this
command from the top-level build directory:

```shell
cmake --build . --target doxygen
```

The web documentation can then be browsed by opening `doc/html/index.html` in
your browser.
