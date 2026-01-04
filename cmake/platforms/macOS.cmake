# macOS-specific CMake configuration for Bolt C++

# Set platform-specific variables
set(BOLT_PLATFORM "macOS")
set(BOLT_PLATFORM_MACOS ON)

# macOS version requirements
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS deployment target")

# macOS-specific compiler settings
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Clang-specific settings for macOS
    add_compile_options(-Wall -Wextra -Wno-unused-parameter)
    
    # Enable modern C++ features
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    
    # Optimization settings
    if(CMAKE_BUILD_TYPE MATCHES Release)
        add_compile_options(-O3 -DNDEBUG)
    elseif(CMAKE_BUILD_TYPE MATCHES Debug)
        add_compile_options(-g -O0)
    endif()
endif()

# macOS-specific frameworks and libraries
set(BOLT_PLATFORM_LIBS
    "-framework Foundation"
    "-framework CoreFoundation"
    "-framework AppKit"
    "-framework CoreGraphics"
    "-framework CoreServices"
    "-framework IOKit"
    "-framework Security"
    "-framework SystemConfiguration"
    "-framework Metal"
    "-framework MetalKit"
    "-framework QuartzCore"
)

# macOS-specific include paths
if(EXISTS "/usr/local/include")
    list(APPEND CMAKE_PREFIX_PATH "/usr/local")
endif()

if(EXISTS "/opt/homebrew")
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
endif()

if(EXISTS "$ENV{VCPKG_ROOT}")
    set(CMAKE_PREFIX_PATH "$ENV{VCPKG_ROOT}/installed/x64-osx;${CMAKE_PREFIX_PATH}")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-osx")
    set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-osx;${CMAKE_PREFIX_PATH}")
endif()

# macOS-specific preprocessor definitions
add_compile_definitions(
    BOLT_PLATFORM_MACOS=1
    _DARWIN_C_SOURCE
    GL_SILENCE_DEPRECATION  # Silence OpenGL deprecation warnings
)

# macOS-specific file extensions
set(BOLT_EXECUTABLE_SUFFIX "")
set(BOLT_LIBRARY_PREFIX "lib")
set(BOLT_LIBRARY_SUFFIX ".dylib")
set(BOLT_STATIC_LIBRARY_SUFFIX ".a")

# macOS-specific installation paths
set(CMAKE_INSTALL_BINDIR "bin")
set(CMAKE_INSTALL_LIBDIR "lib")
set(CMAKE_INSTALL_INCLUDEDIR "include")
set(CMAKE_INSTALL_DATADIR "share")

# macOS-specific app bundle settings
set(CMAKE_MACOSX_BUNDLE TRUE)
set(CMAKE_MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/platforms/Info.plist.in")

# macOS-specific packaging
set(CPACK_GENERATOR "DragNDrop;TGZ")
set(CPACK_DMG_VOLUME_NAME "Bolt C++ IDE")
set(CPACK_DMG_FORMAT "UDZO")
set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/assets/dmg-background.png")

# Code signing (if certificates are available)
if(DEFINED ENV{APPLE_CERTIFICATE_ID})
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "$ENV{APPLE_CERTIFICATE_ID}")
    set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "$ENV{APPLE_DEVELOPMENT_TEAM}")
endif()

# Architecture support
if(CMAKE_OSX_ARCHITECTURES)
    message(STATUS "Building for architectures: ${CMAKE_OSX_ARCHITECTURES}")
else()
    # Default to native architecture
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        set(CMAKE_OSX_ARCHITECTURES "arm64")
    else()
        set(CMAKE_OSX_ARCHITECTURES "x86_64")
    endif()
    message(STATUS "Defaulting to architecture: ${CMAKE_OSX_ARCHITECTURES}")
endif()

# Universal binary support for Apple Silicon and Intel
option(BOLT_BUILD_UNIVERSAL "Build universal binaries for both Intel and Apple Silicon" OFF)
if(BOLT_BUILD_UNIVERSAL)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
    message(STATUS "Building universal binary for arm64 and x86_64")
endif()

# macOS-specific runtime settings
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "@executable_path/../lib;@loader_path/../lib")
set(CMAKE_MACOSX_RPATH TRUE)

message(STATUS "Configured for macOS platform (target: ${CMAKE_OSX_DEPLOYMENT_TARGET})")