# 示例目录CMake配置

# 转换示例程序
add_executable(convert_example ConvertExample.cpp)
target_link_libraries(convert_example PRIVATE MeshFormatConverter ${VTK_LIBRARIES})
target_include_directories(convert_example PRIVATE ${VTK_INCLUDE_DIRS})

# VTK中介格式转换测试程序
add_executable(VTKConverterTest VTKConverterTest.cpp)
target_link_libraries(VTKConverterTest PRIVATE MeshFormatConverter ${VTK_LIBRARIES})
target_include_directories(VTKConverterTest PRIVATE ${VTK_INCLUDE_DIRS})

# MeshReader测试程序
add_executable(MeshReaderTest MeshReaderTest.cpp)
target_link_libraries(MeshReaderTest PRIVATE MeshFormatConverter ${VTK_LIBRARIES})
target_include_directories(MeshReaderTest PRIVATE ${VTK_INCLUDE_DIRS})

# MeshReader CellData测试程序
add_executable(MeshReaderCellDataTest MeshReaderCellDataTest.cpp)
target_link_libraries(MeshReaderCellDataTest PRIVATE MeshFormatConverter ${VTK_LIBRARIES})
target_include_directories(MeshReaderCellDataTest PRIVATE ${VTK_INCLUDE_DIRS})

# 命令行工具
add_executable(meshconv CommandLineTool.cpp)
target_link_libraries(meshconv PRIVATE MeshFormatConverter ${VTK_LIBRARIES})
target_include_directories(meshconv PRIVATE ${VTK_INCLUDE_DIRS})

# 安装示例（可选）
install(TARGETS convert_example VTKConverterTest MeshReaderTest MeshReaderCellDataTest meshconv
    RUNTIME DESTINATION examples
)

