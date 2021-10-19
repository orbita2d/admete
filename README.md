[![Tests - Linux](https://github.com/orbita2d/admete/actions/workflows/cmake-linux.yaml/badge.svg?branch=master)](https://github.com/orbita2d/admete/actions/workflows/cmake-linux.yaml)
[![Tests - Windows](https://github.com/orbita2d/admete/actions/workflows/cmake-windows.yaml/badge.svg?branch=master)](https://github.com/orbita2d/admete/actions/workflows/cmake-windows.yaml)
# admete

αδμετε is a UCI chess engine written in c++.

## Features
 - 64 bit.
 - Single thread.
 - UCI
 - Syzygy tablebases.
 - Variable TT size

## Binaries

Binaries for x86-64 CPUs are available for every release. 

## Build from source

Build from source with cmake with:
```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Tests
Run tests with:

Linux:
```
./build/tests
```

Windows:
```
.\build\Release\tests.exe
```

