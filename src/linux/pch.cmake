#precompiled header generator funcs

function(pathToName result path)
    get_filename_component(path ${path} ABSOLUTE)
    string(REGEX REPLACE "[/ .]" "_" path "${path}")
    set(${result} ${path} PARENT_SCOPE)
endfunction()

function(addPch pch)
    set(pch "${CMAKE_CURRENT_SOURCE_DIR}/${pch}")
    pathToName(pch_target ${pch})
    set(pch_out "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${pch_target}.dir/pch")

    string(TOUPPER CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE} cxx_flags_var)
    set(cxx_flags "${CMAKE_CXX_FLAGS} ${${cxx_flags_var}}")

    get_directory_property(dir_flags INCLUDE_DIRECTORIES)
    foreach(e ${dir_flags})
        list(APPEND cxx_flags "-I${e}")
    endforeach(e)
    
    get_directory_property(dir_flags DEFINITIONS)
    list(APPEND cxx_flags ${dir_flags})

    separate_arguments(cxx_flags)
    add_custom_command(
        OUTPUT "${pch_out}"
        COMMAND ${CMAKE_CXX_COMPILER} -cc1 -x c++ -emit-pch ${CMAKE_PCH_FLAGS} ${cxx_flags} "${pch}" -o "${pch_out}"
        IMPLICIT_DEPENDS CXX "${pch}")
    add_custom_target(${pch_target} DEPENDS "${pch_out}")
endfunction()

function(dependPch target pch)
    set(pch_ "${pch}")
    set(pch "${CMAKE_CURRENT_SOURCE_DIR}/${pch}")
    pathToName(pch_target ${pch})
    set(pch_out "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${pch_target}.dir/pch")

    if(NOT TARGET ${pch_target})
        addPch(${pch_})
    endif()

    add_dependencies(${target} ${pch_target})
    set_property(TARGET ${target} APPEND PROPERTY COMPILE_FLAGS "-include-pch \"${pch_out}\"")
endfunction()
