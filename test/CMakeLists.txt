# Prevent overriding the parent project's compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory(gtest/googletest)

add_executable(unittests
               $<TARGET_OBJECTS:cpp_dependencies_obj>
               ${CMAKE_CURRENT_LIST_DIR}/AnalysisCircularDependencies.cpp)
target_compile_options(unittests
                       PUBLIC ${${PROJECT_NAME}_COMPILE_OPTIONS}
                              ${${PROJECT_NAME}_TEST_COMPILE_OPTIONS})
set_property(TARGET unittests
             APPEND PROPERTY LINK_FLAGS --coverage)
target_include_directories(unittests
                           PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(unittests
                      PUBLIC Boost::filesystem
                             gtest_main)
add_test(NAME unittests
         COMMAND unittests)
