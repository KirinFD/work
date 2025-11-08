# toolchain-aarch64.cmake
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Compiler
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Where you copied your BeagleY-AI's filesystem (sysroot)
# You can extract this using rsync or scp (see step 3)
set(SYSROOT_PATH "/opt/beagley-sysroot")

set(CMAKE_SYSROOT ${SYSROOT_PATH})
set(CMAKE_FIND_ROOT_PATH ${SYSROOT_PATH})

# Search rules for cross-builds
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Ensure pkg-config looks in the target’s pkgconfig directory
set(PKG_CONFIG_EXECUTABLE "aarch64-linux-gnu-pkg-config")
set(ENV{PKG_CONFIG_PATH} "${SYSROOT_PATH}/usr/lib/aarch64-linux-gnu/pkgconfig")
