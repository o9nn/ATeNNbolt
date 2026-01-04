# Doxygen Documentation Generation

# Find Doxygen
find_package(Doxygen OPTIONAL_COMPONENTS dot)

if(DOXYGEN_FOUND)
    message(STATUS "Doxygen found: ${DOXYGEN_EXECUTABLE}")
    
    # Set input and output files
    set(DOXYGEN_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.custom")
    set(DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/docs/api")
    
    # Create output directory
    file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})
    
    # Add documentation target
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CONFIG_FILE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
    
    # Add docs-clean target
    add_custom_target(docs-clean
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${DOXYGEN_OUTPUT_DIR}/html
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${DOXYGEN_OUTPUT_DIR}/latex
        COMMENT "Cleaning generated documentation"
    )
    
    if(DOXYGEN_DOT_FOUND)
        message(STATUS "Graphviz dot found: ${DOXYGEN_DOT_EXECUTABLE}")
        message(STATUS "Documentation will include diagrams")
    else()
        message(STATUS "Graphviz dot not found - diagrams will be disabled")
    endif()
    
    message(STATUS "Documentation target 'docs' available")
    message(STATUS "Run 'make docs' to generate API documentation")
    message(STATUS "Output will be in: ${DOXYGEN_OUTPUT_DIR}/html/index.html")
else()
    message(STATUS "Doxygen not found - documentation target not available")
    message(STATUS "Install Doxygen: sudo apt-get install doxygen graphviz")
endif()
