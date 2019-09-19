# useful utilities

function(redefine_file_macro targetname)
    add_definitions(-Wno-builtin-macro-redefined)
    # Get source files of target
    get_target_property(source_files "${targetname}" SOURCES)
    foreach(sourcefile ${source_files})
        # Get compile definitions in source file
        get_property(defs SOURCE "${sourcefile}" PROPERTY COMPILE_DEFINITIONS)
        # Get absolute path of source file
        get_filename_component(filepath "${sourcefile}" ABSOLUTE)
        # Trim leading dir according to ${PROJECT_SOURCE_DIR}
        string(REPLACE ${PROJECT_SOURCE_DIR}/ "" relpath ${filepath})
        # Add __FILE__ definition to compile definitions
        list(APPEND defs "__FILE__=\"${relpath}\"")
        # Set compile definitions to property
        set_property(SOURCE "${sourcefile}" PROPERTY COMPILE_DEFINITIONS ${defs})
    endforeach()
endfunction()