# Linux-specific CMake configuration for Bolt C++

# Set platform-specific variables
set(BOLT_PLATFORM "Linux")
set(BOLT_PLATFORM_LINUX ON)

# Linux distribution detection
if(EXISTS "/etc/os-release")
    file(STRINGS "/etc/os-release" OS_RELEASE)
    foreach(LINE ${OS_RELEASE})
        if(LINE MATCHES "^ID=(.+)")
            set(LINUX_DISTRO ${CMAKE_MATCH_1})
            string(REPLACE "\"" "" LINUX_DISTRO ${LINUX_DISTRO})
        endif()
    endforeach()
    message(STATUS "Detected Linux distribution: ${LINUX_DISTRO}")
endif()

# Linux-specific compiler settings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # GCC-specific settings
    add_compile_options(-Wall -Wextra -Wno-unused-parameter)
    
    # Enable GNU extensions
    add_compile_definitions(_GNU_SOURCE)
    
    # Optimization settings
    if(CMAKE_BUILD_TYPE MATCHES Release)
        add_compile_options(-O3 -DNDEBUG -march=native)
    elseif(CMAKE_BUILD_TYPE MATCHES Debug)
        add_compile_options(-g -O0 -fno-omit-frame-pointer)
    endif()
    
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Clang-specific settings
    add_compile_options(-Wall -Wextra -Wno-unused-parameter)
    
    if(CMAKE_BUILD_TYPE MATCHES Release)
        add_compile_options(-O3 -DNDEBUG)
    elseif(CMAKE_BUILD_TYPE MATCHES Debug)
        add_compile_options(-g -O0)
    endif()
endif()

# Linux-specific libraries
set(BOLT_PLATFORM_LIBS
    pthread     # POSIX threads
    dl          # Dynamic linking loader
    rt          # Real-time extensions
    m           # Math library
    z           # Compression library
    ssl         # OpenSSL
    crypto      # Cryptography
)

# Linux-specific system library detection
find_package(PkgConfig QUIET)

# X11 support for GUI applications
find_package(X11 QUIET)
if(X11_FOUND)
    list(APPEND BOLT_PLATFORM_LIBS ${X11_LIBRARIES})
    add_compile_definitions(BOLT_HAVE_X11=1)
endif()

# Wayland support (alternative to X11)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(WAYLAND QUIET wayland-client wayland-cursor wayland-egl)
    if(WAYLAND_FOUND)
        list(APPEND BOLT_PLATFORM_LIBS ${WAYLAND_LIBRARIES})
        add_compile_definitions(BOLT_HAVE_WAYLAND=1)
    endif()
endif()

# Linux-specific include paths
list(APPEND CMAKE_PREFIX_PATH 
    "/usr/local"
    "/usr"
    "/opt"
)

if(EXISTS "$ENV{VCPKG_ROOT}")
    set(CMAKE_PREFIX_PATH "$ENV{VCPKG_ROOT}/installed/x64-linux;${CMAKE_PREFIX_PATH}")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-linux")
    set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-linux;${CMAKE_PREFIX_PATH}")
endif()

# Linux-specific preprocessor definitions
add_compile_definitions(
    BOLT_PLATFORM_LINUX=1
    _POSIX_C_SOURCE=200809L
    _DEFAULT_SOURCE
)

# Linux-specific file extensions
set(BOLT_EXECUTABLE_SUFFIX "")
set(BOLT_LIBRARY_PREFIX "lib")
set(BOLT_LIBRARY_SUFFIX ".so")
set(BOLT_STATIC_LIBRARY_SUFFIX ".a")

# Linux-specific installation paths
include(GNUInstallDirs)
set(CMAKE_INSTALL_BINDIR "${CMAKE_INSTALL_BINDIR}")
set(CMAKE_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}")
set(CMAKE_INSTALL_DATADIR "${CMAKE_INSTALL_DATADIR}")

# Desktop integration
set(CMAKE_INSTALL_DESKTOPDIR "${CMAKE_INSTALL_DATADIR}/applications")
set(CMAKE_INSTALL_ICONDIR "${CMAKE_INSTALL_DATADIR}/icons/hicolor")

# Linux-specific packaging
set(CPACK_GENERATOR "DEB;RPM;TGZ")

# Debian/Ubuntu packaging
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libcurl4,libjsoncpp25,libglfw3,libgl1,libssl3")
set(CPACK_DEBIAN_PACKAGE_SECTION "development")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "EchoCog <support@echocog.com>")

# RPM/Fedora packaging
set(CPACK_RPM_PACKAGE_REQUIRES "libcurl >= 7.0, jsoncpp >= 1.9, glfw >= 3.3, mesa-libGL")
set(CPACK_RPM_PACKAGE_GROUP "Development/Tools")
set(CPACK_RPM_PACKAGE_LICENSE "MIT")

# Runtime library paths
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib:\$ORIGIN/lib:/usr/local/lib")

# Distribution-specific optimizations
if(LINUX_DISTRO MATCHES "ubuntu|debian")
    # Ubuntu/Debian specific settings
    add_compile_definitions(BOLT_DISTRO_DEBIAN=1)
elseif(LINUX_DISTRO MATCHES "fedora|centos|rhel")
    # Red Hat family specific settings
    add_compile_definitions(BOLT_DISTRO_REDHAT=1)
elseif(LINUX_DISTRO MATCHES "arch")
    # Arch Linux specific settings
    add_compile_definitions(BOLT_DISTRO_ARCH=1)
endif()

message(STATUS "Configured for Linux platform (${LINUX_DISTRO})")