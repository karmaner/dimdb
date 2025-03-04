# /tests/CMakeLists.txt

# 设置测试源文件
set(TEST_SOURCES
  common/value_test.cpp
  storage/buffer/lru_replacer_test.cpp
)

# 添加测试可执行文件
foreach(test_source ${TEST_SOURCES})
  # 获取文件名（不含扩展名）作为测试名
  get_filename_component(test_name ${test_source} NAME_WE)
  
  # 添加可执行文件
  add_executable(${test_name} ${test_source})
  
  # 添加包含目录
  target_include_directories(${test_name}
    PRIVATE
      ${CMAKE_SOURCE_DIR}/src/dimserver
      ${CMAKE_SOURCE_DIR}/3rd/google/googletest/include
  )
  
  # 链接必要的库
  target_link_libraries(${test_name}
    PRIVATE
      dimdb_common
      gtest
      gtest_main
      pthread
  )
  
  # 添加到 CTest
  add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# 启用测试覆盖率报告（可选）
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
  endif()
endif()
