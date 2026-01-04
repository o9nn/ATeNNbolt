# Cross-Platform Detection and Configuration for Bolt C++

# Detect the target platform
if(WIN32)
    set(BOLT_TARGET_PLATFORM "Windows")
elseif(APPLE)
    set(BOLT_TARGET_PLATFORM "macOS")
elseif(UNIX)
    set(BOLT_TARGET_PLATFORM "Linux")
else()
    set(BOLT_TARGET_PLATFORM "Unknown")
endif()

message(STATUS "Detected platform: ${BOLT_TARGET_PLATFORM}")

# Include platform-specific configuration
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/platforms/${BOLT_TARGET_PLATFORM}.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/platforms/${BOLT_TARGET_PLATFORM}.cmake")
    message(STATUS "Loaded platform configuration: ${BOLT_TARGET_PLATFORM}")
else()
    message(WARNING "No platform-specific configuration found for ${BOLT_TARGET_PLATFORM}")
endif()

# Cross-platform feature detection
include(CheckIncludeFileCXX)
include(CheckCXXSymbolExists)

# Check for platform-specific headers using C++
check_include_file_cxx("windows.h" HAVE_WINDOWS_H)
check_include_file_cxx("unistd.h" HAVE_UNISTD_H)
check_include_file_cxx("sys/types.h" HAVE_SYS_TYPES_H)
check_include_file_cxx("sys/stat.h" HAVE_SYS_STAT_H)
check_include_file_cxx("dirent.h" HAVE_DIRENT_H)

# Check for platform-specific functions
check_cxx_symbol_exists("CreateDirectory" "windows.h" HAVE_CREATE_DIRECTORY)
check_cxx_symbol_exists("mkdir" "sys/stat.h;unistd.h" HAVE_MKDIR)
check_cxx_symbol_exists("dlopen" "dlfcn.h" HAVE_DLOPEN)
check_cxx_symbol_exists("LoadLibrary" "windows.h" HAVE_LOAD_LIBRARY)

# Generate platform configuration header
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/config/platform_config.h.in"
    "${CMAKE_BINARY_DIR}/include/bolt/platform_config.h"
    @ONLY
)

# Add the generated header to the include path
include_directories("${CMAKE_BINARY_DIR}/include")

# Platform-specific compiler definitions
if(BOLT_PLATFORM_WINDOWS)
    add_compile_definitions(BOLT_WINDOWS=1)
elseif(BOLT_PLATFORM_MACOS)
    add_compile_definitions(BOLT_MACOS=1)
elseif(BOLT_PLATFORM_LINUX)
    add_compile_definitions(BOLT_LINUX=1)
endif()

# Cross-platform threading support
find_package(Threads REQUIRED)
if(Threads_FOUND)
    set(BOLT_HAVE_THREADS TRUE)
    add_compile_definitions(BOLT_HAVE_THREADS=1)
endif()

# Cross-platform filesystem support
if(CMAKE_CXX_STANDARD GREATER_EQUAL 17)
    set(BOLT_HAVE_FILESYSTEM TRUE)
    add_compile_definitions(BOLT_HAVE_FILESYSTEM=1)
endif()

# Platform-specific optimization flags
option(BOLT_ENABLE_LTO "Enable Link Time Optimization" OFF)
if(BOLT_ENABLE_LTO AND NOT CMAKE_BUILD_TYPE MATCHES Debug)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported)
    if(ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
        message(STATUS "Link Time Optimization enabled")
    endif()
endif()

# Platform-specific debug information
if(CMAKE_BUILD_TYPE MATCHES Debug)
    if(BOLT_PLATFORM_WINDOWS AND MSVC)
        add_compile_options(/Zi)
    elseif(BOLT_PLATFORM_LINUX OR BOLT_PLATFORM_MACOS)
        add_compile_options(-g3 -ggdb)
    endif()
endif()

# Cross-platform shared library handling
macro(bolt_add_library target_name)
    add_library(${target_name} ${ARGN})
    
    # Platform-specific library settings
    if(BOLT_PLATFORM_WINDOWS)
        set_target_properties(${target_name} PROPERTIES
            WINDOWS_EXPORT_ALL_SYMBOLS ON
            PREFIX ""
        )
    elseif(BOLT_PLATFORM_LINUX OR BOLT_PLATFORM_MACOS)
        set_target_properties(${target_name} PROPERTIES
            POSITION_INDEPENDENT_CODE ON
        )
    endif()
    
    # Link platform-specific libraries
    if(BOLT_PLATFORM_LIBS)
        target_link_libraries(${target_name} PRIVATE ${BOLT_PLATFORM_LIBS})
    endif()
endmacro()

# Cross-platform executable handling
macro(bolt_add_executable target_name)
    add_executable(${target_name} ${ARGN})
    
    # Platform-specific executable settings
    if(BOLT_PLATFORM_WINDOWS)
        set_target_properties(${target_name} PROPERTIES
            WIN32_EXECUTABLE ON
        )
        if(BOLT_GUI_APPLICATION)
            set_target_properties(${target_name} PROPERTIES
                LINK_FLAGS "/SUBSYSTEM:WINDOWS"
            )
        endif()
    elseif(BOLT_PLATFORM_MACOS)
        if(BOLT_GUI_APPLICATION)
            set_target_properties(${target_name} PROPERTIES
                MACOSX_BUNDLE ON
                MACOSX_BUNDLE_BUNDLE_NAME "${target_name}"
                MACOSX_BUNDLE_GUI_IDENTIFIER "com.echocog.${target_name}"
            )
        endif()
    endif()
    
    # Link platform-specific libraries
    if(BOLT_PLATFORM_LIBS)
        target_link_libraries(${target_name} PRIVATE ${BOLT_PLATFORM_LIBS})
    endif()
endmacro()

message(STATUS "Cross-platform configuration completed for ${BOLT_TARGET_PLATFORM}")