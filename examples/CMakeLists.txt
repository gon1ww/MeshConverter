# 示例目录CMake配置

# 单文件转换示例
add_executable(convert_example ConvertExample.cpp)
target_link_libraries(convert_example PRIVATE MeshFormatConverter)

# 安装示例（可选）
install(TARGETS convert_example
    RUNTIME DESTINATION examples
)
