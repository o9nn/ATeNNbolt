# Windows-specific CMake configuration for Bolt C++

# Set platform-specific variables
set(BOLT_PLATFORM "Windows")
set(BOLT_PLATFORM_WINDOWS ON)

# Windows-specific compiler settings
if(MSVC)
    # MSVC-specific settings
    add_compile_definitions(
        _WIN32_WINNT=0x0601  # Windows 7 and later
        NOMINMAX             # Don't define min/max macros
        WIN32_LEAN_AND_MEAN  # Exclude rarely-used stuff from Windows headers
        _CRT_SECURE_NO_WARNINGS  # Disable CRT security warnings
    )
    
    # Enable parallel compilation
    add_compile_options(/MP)
    
    # Set warning level
    add_compile_options(/W3)
    
    # Runtime library settings
    if(CMAKE_BUILD_TYPE MATCHES Debug)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
    else()
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    endif()
    
elseif(MINGW)
    # MinGW-specific settings
    add_compile_options(-mwindows)
    add_link_options(-mwindows)
    
    # Static linking for portability
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif()

# Windows-specific libraries
set(BOLT_PLATFORM_LIBS
    ws2_32      # Winsock 2
    winmm       # Windows Multimedia
    shlwapi     # Shell API
    advapi32    # Advanced API
    user32      # User interface
    kernel32    # Kernel
    ole32       # OLE
    oleaut32    # OLE Automation
    uuid        # UUID generation
    gdi32       # Graphics Device Interface
    comctl32    # Common Controls
)

# Windows-specific include paths
if(EXISTS "$ENV{VCPKG_ROOT}")
    set(CMAKE_PREFIX_PATH "$ENV{VCPKG_ROOT}/installed/x64-windows;${CMAKE_PREFIX_PATH}")
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows")
    set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows;${CMAKE_PREFIX_PATH}")
endif()

# Windows-specific preprocessor definitions
add_compile_definitions(
    BOLT_PLATFORM_WINDOWS=1
    UNICODE
    _UNICODE
)

# Windows-specific file extensions
set(BOLT_EXECUTABLE_SUFFIX ".exe")
set(BOLT_LIBRARY_PREFIX "")
set(BOLT_LIBRARY_SUFFIX ".dll")
set(BOLT_STATIC_LIBRARY_SUFFIX ".lib")

# Windows-specific installation paths
set(CMAKE_INSTALL_BINDIR "bin")
set(CMAKE_INSTALL_LIBDIR "lib")
set(CMAKE_INSTALL_INCLUDEDIR "include")
set(CMAKE_INSTALL_DATADIR "share")

# Windows-specific packaging
set(CPACK_GENERATOR "NSIS;ZIP")
set(CPACK_NSIS_DISPLAY_NAME "Bolt C++ IDE")
set(CPACK_NSIS_PACKAGE_NAME "Bolt-CPP")
set(CPACK_NSIS_CONTACT "support@echocog.com")
set(CPACK_NSIS_HELP_LINK "https://github.com/EchoCog/bolt-cppml")
set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/EchoCog/bolt-cppml")
set(CPACK_NSIS_MODIFY_PATH ON)

message(STATUS "Configured for Windows platform")