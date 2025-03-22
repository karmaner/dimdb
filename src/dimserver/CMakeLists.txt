file(GLOB_RECURSE dimserver_src "*.cpp")
add_library(dimserver STATIC ${dimserver_src})
target_include_directories(dimserver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dimserver PUBLIC ${Boost_LIBRARIES})

