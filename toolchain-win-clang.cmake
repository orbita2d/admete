# toolchain-win-clang.cmake
# Cross-compile to Windows (x86_64) using clang as the code generator, reusing the
# mingw-w64 installation purely as a sysroot (headers + libstdc++). Pair with lld,
# whose MinGW mode emits PE/COFF and supports ThinLTO.
SET(CMAKE_SYSTEM_NAME Windows)

SET(CMAKE_C_COMPILER clang)
SET(CMAKE_CXX_COMPILER clang++)
SET(CMAKE_C_COMPILER_TARGET x86_64-w64-mingw32)
SET(CMAKE_CXX_COMPILER_TARGET x86_64-w64-mingw32)
SET(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# clang's mingw driver borrows the GCC sysroot for this triple; on Debian/Ubuntu it
# lives here. If clang can't find the headers, add --sysroot=${MINGW_ROOT} below.
SET(MINGW_ROOT /usr/x86_64-w64-mingw32)
SET(CMAKE_FIND_ROOT_PATH ${MINGW_ROOT})
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
