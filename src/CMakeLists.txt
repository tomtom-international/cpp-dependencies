add_library(cpp_dependencies_obj OBJECT
            ${CMAKE_CURRENT_LIST_DIR}/Analysis.cpp
            ${CMAKE_CURRENT_LIST_DIR}/CmakeRegen.cpp
            ${CMAKE_CURRENT_LIST_DIR}/Component.cpp
            ${CMAKE_CURRENT_LIST_DIR}/Configuration.cpp
            ${CMAKE_CURRENT_LIST_DIR}/generated.cpp
            ${CMAKE_CURRENT_LIST_DIR}/Input.cpp
            ${CMAKE_CURRENT_LIST_DIR}/Output.cpp)
target_compile_options(cpp_dependencies_obj
                       PUBLIC ${${PROJECT_NAME}_COMPILE_OPTIONS})

add_executable(cpp-dependencies
               ${CMAKE_CURRENT_LIST_DIR}/main.cpp
               $<TARGET_OBJECTS:cpp_dependencies_obj>)
target_compile_options(cpp-dependencies
                       PUBLIC ${${PROJECT_NAME}_COMPILE_OPTIONS})
target_link_libraries(cpp-dependencies
                      PUBLIC Boost::filesystem)
target_include_directories(cpp-dependencies
                           PUBLIC ${CMAKE_CURRENT_LIST_DIR})

install(TARGETS cpp-dependencies
        RUNTIME DESTINATION bin)
