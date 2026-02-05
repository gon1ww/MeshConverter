# 网格格式转换工具（MeshFormatConverter）

## 项目简介

MeshFormatConverter是一个功能强大的网格格式转换工具，支持多种网格格式的读写和转换，为CFD/FEA流程提供格式转换能力。

## 支持的格式

### 体网格格式
- VTK Legacy (.vtk)
- VTK XML (.vtu/.vtp/.vti/.vts)
- CGNS (.cgns)
- Gmsh v2/v4 (.msh)
- SU2 (.su2)
- OpenFOAM (foamFile)

### 面网格格式
- STL ASCII/Binary (.stl)
- OBJ (.obj)
- PLY ASCII/Binary (.ply)
- OFF (.off)

## 技术栈

- 开发语言：C++17
- 核心依赖：VTK 9.x、CGNS API 4.x、Gmsh SDK 4.x、CMake 3.20+
- 跨平台支持：Windows/Linux/macOS
- 异常处理：基于C++异常体系，自定义异常类型
- 构建系统：CMake
- 测试框架：内置测试工具

## 项目结构

```
MeshFormatConverter/
├── include/          # 头文件
├── src/              # 源文件
├── examples/         # 使用示例和测试工具
├── data/             # 测试数据
├── extern/           # 外部依赖
├── build/            # 构建目录
├── build_new/        # 替代构建目录
├── CMakeLists.txt    # CMake配置
└── README.md         # 项目说明
```

## 构建指南

### 前提条件

- CMake 3.20+
- C++17兼容的编译器
- VTK 9.0+
- CGNS API 4.0+ (可选)
- Gmsh SDK 4.1+ (可选)

### 构建步骤

1. **配置项目**
   ```bash
   mkdir build
   cd build
   cmake ..
   ```

2. **构建项目**
   ```bash
   # Windows
   cmake --build . --config Debug
   # 或
   cmake --build . --config Release
   
   # Linux/macOS
   cmake --build .
   ```

3. **运行测试**
   ```bash
   # 使用内置测试工具
   bin/Debug/MeshReaderTest.exe data/src/stl/aneurysm_data.stl
   ```

4. **安装项目** (可选)
   ```bash
   cmake --install .
   ```

## 使用示例

### 单文件转换

```cpp
#include "MeshConverter.h"
#include "MeshHelper.h"

int main() {
    std::string srcPath = "input.vtk";
    std::string dstPath = "output.cgns";
    MeshFormat srcFormat = MeshHelper::detectFormat(srcPath);
    MeshFormat dstFormat = MeshFormat::CGNS;
    FormatWriteOptions writeOptions;
    
    writeOptions.cgnsBaseName = "CFD_Base";
    writeOptions.cgnsZoneName = "Flow_Zone";
    writeOptions.isBinary = true;

    MeshErrorCode errorCode;
    std::string errorMsg;
    bool success = MeshConverter::convert(srcPath, dstPath, srcFormat, dstFormat, writeOptions, errorCode, errorMsg);

    if (success) {
        std::cout << "转换成功！" << std::endl;
    } else {
        std::cerr << "转换失败：" << errorMsg << std::endl;
    }

    return success ? 0 : 1;
}
```

### 批量转换

```cpp
#include "MeshConverter.h"

int main() {
    std::vector<std::string> srcFiles = {"mesh1.vtk", "mesh2.msh", "mesh3.stl"};
    std::string dstDir = "output";
    MeshFormat dstFormat = MeshFormat::VTK_XML;
    FormatWriteOptions writeOptions;
    
    std::unordered_map<std::string, std::pair<MeshErrorCode, std::string>> errorMap;
    uint64_t successCount = MeshConverter::batchConvert(srcFiles, dstDir, dstFormat, writeOptions, errorMap);

    std::cout << "成功转换：" << successCount << " 个文件" << std::endl;
    if (!errorMap.empty()) {
        std::cout << "失败文件：" << errorMap.size() << " 个" << std::endl;
        for (const auto& [file, error] : errorMap) {
            std::cerr << file << ": " << error.second << std::endl;
        }
    }

    return 0;
}
```

### 命令行工具使用

项目提供了 `meshconv` 命令行工具，用于快速执行网格格式转换：

```bash
# 基本用法
meshconv <input_file> <output_file>

# 示例：将STL文件转换为VTK文件
meshconv input.stl output.vtk

# 示例：将VTK文件转换为CGNS文件
meshconv input.vtk output.cgns
```

### 测试工具

项目包含多个测试工具，用于验证网格读取和转换功能：

- **MeshReaderTest.exe**：测试网格文件读取功能
- **STLReaderTest.exe**：专门测试STL文件读取
- **VTKConverterTest.exe**：测试VTK文件转换

使用示例：

```bash
# 测试STL文件读取
MeshReaderTest.exe data/src/stl/aneurysm_data.stl

# 测试VTK文件转换
VTKConverterTest.exe data/src/vtk/legacy/unstructuredTime1.vtk
```

## API 文档

### 核心接口

- **MeshReader**：网格读取模块，支持自动格式识别和指定格式读取，已修复STL文件格式检测和内存分配问题
- **MeshWriter**：网格写入模块，支持格式特异性配置
- **MeshConverter**：格式转换模块，提供单文件和批量转换功能
- **MeshProcessor**：网格处理模块，提供表面提取、网格校验等功能
- **MeshHelper**：辅助接口模块，提供格式识别、元数据提取等功能
- **VTKConverter**：VTK格式转换模块，支持不同VTK格式之间的转换

### 数据结构

- **MeshData**：网格核心数据，包含几何、拓扑和属性数据
- **MeshMetadata**：网格元数据，描述网格属性
- **FormatWriteOptions**：格式写入选项，支持格式特异性配置
- **MeshException**：异常处理类，支持错误码和错误信息
- **MeshErrorCode**：错误码枚举，定义各种错误类型
- **MeshFormat**：格式枚举，定义支持的网格格式

## 扩展指南

### 新增网格格式

1. 在 `MeshFormat` 枚举中添加新格式值
2. 实现 `MeshReader` 的 `readXXX` 方法
3. 实现 `MeshWriter` 的 `writeXXX` 方法
4. 在 `MeshHelper::detectFormat` 中添加格式识别逻辑
5. 补充 `FormatWriteOptions` 中的格式特异性配置
6. 编写单元测试

### 新增网格处理功能

1. 在 `MeshProcessor` 中添加新方法
2. 实现处理逻辑
3. 编写单元测试

## 故障排除

### 常见错误

- **依赖库缺失**：确保所有依赖库已正确安装并配置
- **格式不支持**：检查文件格式是否在支持列表中
- **文件不存在**：确认文件路径正确
- **权限不足**：确保有文件读写权限
- **STL文件读取错误**：如果遇到"bad allocation"错误，可能是因为文件格式检测错误。项目已修复此问题，现在会正确检测STL ASCII和二进制格式。
- **VTK文件读取错误**：对于结构化网格和曲线网格，项目已添加支持，能够正确转换为非结构化网格。

### 调试建议

- 启用详细日志
- 检查输入文件格式
- 验证依赖库版本
- 使用 `MeshHelper::extractMetadata` 检查文件元数据
- 对于STL文件，确保文件格式正确（ASCII或二进制）
- 对于大型网格文件，确保有足够的内存

### 已知问题与解决方案

- **STL文件格式检测**：项目已修复STL文件格式检测逻辑，现在会根据文件内容而不仅仅是扩展名来判断格式。
- **二进制STL内存分配**：添加了三角形数量验证和内存分配异常捕获，防止因异常大的三角形数量导致的内存分配失败。
- **VTK结构化网格支持**：添加了对结构化网格和曲线网格的支持，能够正确转换为非结构化网格。

## 许可证

MIT License

## 联系方式

如有问题或建议，请联系项目维护者。
