

if(MSVC)
    file(GLOB_RECURSE tiny_dnn_headers "${CMAKE_SOURCE_DIR}/tiny_dnn/*.h")
    file(RELATIVE_PATH tiny_dnn_relpath ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_SOURCE_DIR}/tiny_dnn")
    string(REPLACE "/" "\\" tiny_dnn_relpath "${tiny_dnn_relpath}")
    foreach(_source IN ITEMS ${tiny_dnn_headers})
        file(RELATIVE_PATH _source_rel ${CMAKE_CURRENT_SOURCE_DIR} "${_source}")
        get_filename_component(_source_path "${_source_rel}" PATH)
        string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
        string(REPLACE ${tiny_dnn_relpath} "tiny_dnn" _source_path_msvc ${_source_path_msvc})
        source_group("${_source_path_msvc}" FILES "${_source}")
    endforeach()
    file(GLOB_RECURSE xoxo_headers "${CMAKE_SOURCE_DIR}/xoxo/tiny_dnn_xoxo/*.h")
endif()
file(GLOB_RECURSE xoxo_cpp "${CMAKE_SOURCE_DIR}/xoxo/tiny_dnn_xoxo/*.cpp")
file(GLOB_RECURSE xoxo_c "${CMAKE_SOURCE_DIR}/xoxo/tiny_dnn_xoxo/*.c")

	
add_executable(tiny_dnn_xoxo_play ${xoxo_headers} ${xoxo_cpp} ${xoxo_c} ${tiny_dnn_headers})
target_link_libraries(tiny_dnn_xoxo_play
    ${project_library_target_name} ${REQUIRED_LIBRARIES})

