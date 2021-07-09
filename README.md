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
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## Tests
Run tests with:

Linux:
```
./build/tests`
```

Windows:
```
.\build\Release\tests.exe
```

