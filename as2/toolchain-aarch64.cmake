# Cross toolchain file for Linux aarch64 (ARM64)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Cross compilers
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Point this to your target sysroot (rootfs) that contains /usr/include, /usr/lib, pkgconfig, etc.
# Example: a debootstrap rootfs, Yocto SDK sysroot, or an rsync of your device’s root.
# Replace this path with yours:
set(CMAKE_SYSROOT /opt/sysroots/aarch64-linux-gnu)

# Tell CMake to search headers/libs in the sysroot
set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Ensure pkg-config reads target .pc files from the sysroot
# Debian/Ubuntu multiarch layout typically uses /usr/lib/aarch64-linux-gnu/pkgconfig
set(ENV{PKG_CONFIG_SYSROOT_DIR} ${CMAKE_SYSROOT})
set(ENV{PKG_CONFIG_LIBDIR}
    ${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu/pkgconfig:
    ${CMAKE_SYSROOT}/usr/lib/pkgconfig:
    ${CMAKE_SYSROOT}/usr/share/pkgconfig)
# Avoid host .pc files
set(ENV{PKG_CONFIG_PATH} "")
# Optional: if you have a cross pkg-config wrapper
# set(PKG_CONFIG_EXECUTABLE aarch64-linux-gnu-pkg-config)