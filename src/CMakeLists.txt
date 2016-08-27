add_library(cpp_dependencies_obj OBJECT
            ${CMAKE_CURRENT_LIST_DIR}/Analysis.h
            ${CMAKE_CURRENT_LIST_DIR}/CmakeRegen.h
            ${CMAKE_CURRENT_LIST_DIR}/Component.h
            ${CMAKE_CURRENT_LIST_DIR}/Configuration.h
            ${CMAKE_CURRENT_LIST_DIR}/Constants.h
            ${CMAKE_CURRENT_LIST_DIR}/Input.h
            ${CMAKE_CURRENT_LIST_DIR}/Output.h

            ${CMAKE_CURRENT_LIST_DIR}/Analysis.cpp
            ${CMAKE_CURRENT_LIST_DIR}/CmakeRegen.cpp
            ${CMAKE_CURRENT_LIST_DIR}/Component.cpp
            ${CMAKE_CURRENT_LIST_DIR}/Configuration.cpp
            ${CMAKE_CURRENT_LIST_DIR}/generated.cpp
            ${CMAKE_CURRENT_LIST_DIR}/Input.cpp
            ${CMAKE_CURRENT_LIST_DIR}/Output.cpp)
target_compile_options(cpp_dependencies_obj
                       PUBLIC ${${PROJECT_NAME}_COMPILE_OPTIONS})
target_include_directories(cpp_dependencies_obj
                           PUBLIC ${Boost_INCLUDE_DIR})

add_executable(cpp-dependencies
               ${CMAKE_CURRENT_LIST_DIR}/main.cpp
               $<TARGET_OBJECTS:cpp_dependencies_obj>)
target_compile_options(cpp-dependencies
                       PUBLIC ${${PROJECT_NAME}_COMPILE_OPTIONS})
target_link_libraries(cpp-dependencies
                      PUBLIC ${Boost_LIBRARIES})
target_include_directories(cpp-dependencies
                           PUBLIC "${CMAKE_CURRENT_LIST_DIR}"
                                  "${Boost_INCLUDE_DIRS}")

install(TARGETS cpp-dependencies
        RUNTIME DESTINATION bin)
