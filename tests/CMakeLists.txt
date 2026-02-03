# 测试目录CMake配置

# GTest已经在主CMakeLists.txt中配置
include(GoogleTest)

# 单元测试
add_executable(unit_tests
    unit/MeshTypesTest.cpp
    unit/MeshReaderTest.cpp
    unit/MeshWriterTest.cpp
    unit/MeshConverterTest.cpp
    unit/MeshProcessorTest.cpp
    unit/MeshHelperTest.cpp
    unit/MeshExceptionTest.cpp
)

# 集成测试
add_executable(integration_tests
    integration/FormatConversionTest.cpp
    integration/MultiFormatTest.cpp
)

# 性能测试
add_executable(performance_tests
    performance/LargeFileTest.cpp
    performance/BatchConvertTest.cpp
)

# 链接依赖
target_link_libraries(unit_tests PRIVATE MeshFormatConverter GTest::GTest GTest::Main)
target_link_libraries(integration_tests PRIVATE MeshFormatConverter GTest::GTest GTest::Main)
target_link_libraries(performance_tests PRIVATE MeshFormatConverter GTest::GTest GTest::Main)

# 添加测试
gtest_discover_tests(unit_tests)
gtest_discover_tests(integration_tests)
gtest_discover_tests(performance_tests)

# 测试数据目录
set(TEST_DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/data)
add_definitions(-DTEST_DATA_DIR="${TEST_DATA_DIR}")

# 确保测试数据目录存在
file(MAKE_DIRECTORY ${TEST_DATA_DIR})
