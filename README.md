# MeshFormatConverter

<p align="center">
  <img src="https://trae-api-cn.mchost.guru/api/ide/v1/text_to_image?prompt=3D%20mesh%20converter%20logo%20with%20geometric%20shapes%20and%20file%20formats%20icons%2C%20professional%20and%20modern%20design%2C%20blue%20and%20green%20color%20scheme&image_size=square_hd" alt="MeshFormatConverter Logo" width="200" height="200">
</p>

<p align="center">
  <a href="https://github.com/yourusername/MeshFormatConverter">
    <img src="https://img.shields.io/github/stars/yourusername/MeshFormatConverter.svg" alt="GitHub stars">
  </a>
  <a href="https://github.com/yourusername/MeshFormatConverter/blob/main/LICENSE">
    <img src="https://img.shields.io/github/license/yourusername/MeshFormatConverter.svg" alt="License">
  </a>
  <a href="https://github.com/yourusername/MeshFormatConverter">
    <img src="https://img.shields.io/github/languages/top/yourusername/MeshFormatConverter.svg" alt="Top language">
  </a>
  <a href="https://github.com/yourusername/MeshFormatConverter">
    <img src="https://img.shields.io/github/last-commit/yourusername/MeshFormatConverter.svg" alt="Last commit">
  </a>
</p>

## 项目概述

MeshFormatConverter 是一款功能强大的网格格式转换工具，专为 CFD (计算流体力学) 和 FEA (有限元分析) 领域设计。该工具支持多种网格格式的读写和转换，为工程仿真流程提供了便捷的格式转换能力。

- **多格式支持**：支持 10+ 种主流网格格式的相互转换
- **高性能**：基于 VTK 引擎，提供高效的网格处理能力
- **用户友好**：提供命令行工具和 Qt GUI 应用程序
- **跨平台**：支持 Windows、Linux 和 macOS 操作系统
- **可扩展**：模块化设计，易于添加新的网格格式和处理功能

## 核心功能

### 格式转换
- **多格式互转**：支持 VTK、CGNS、Gmsh、STL、OBJ、PLY、OFF 等格式的相互转换
- **批量转换**：支持批量处理多个网格文件
- **格式自动检测**：自动识别输入文件格式，无需手动指定
- **格式特异性配置**：针对不同格式提供专用配置选项

### 网格处理
- **表面提取**：从体网格中提取表面网格
- **网格校验**：验证网格质量和完整性
- **元数据提取**：提取和处理网格元数据
- **几何信息计算**：计算网格的基本几何信息

### 可视化与分析
- **3D 视图**：通过 QtTransformApp 提供网格的 3D 可视化
- **网格信息查看**：详细展示网格的基本信息、单元统计和属性信息
- **实时预览**：在导出前预览转换结果

### 用户界面
- **命令行工具**：快速执行格式转换操作
- **Qt GUI 应用**：提供直观的图形界面，支持拖拽操作和实时预览
- **导出终端**：实时显示导出进度和结果

## 技术栈

| 类别 | 技术/库 | 版本 | 用途 |
|------|---------|------|------|
| 开发语言 | C++ | C++17 | 核心功能实现 |
| 构建系统 | CMake | 3.20+ | 项目构建和配置 |
| 核心依赖 | VTK | 9.5+ | 网格处理和格式转换 |
| 可选依赖 | CGNS | 4.0+ | CGNS 格式支持 |
| 可选依赖 | Gmsh | 4.1+ | Gmsh 格式支持 |
| GUI 框架 | Qt | 6.10+ | 图形界面应用 |
| 测试框架 | 内置测试工具 | - | 功能验证 |
| 文档生成 | Doxygen | - | API 文档生成 |

## 环境要求

### 硬件要求
- **CPU**：至少 2 核处理器
- **内存**：至少 4GB RAM（处理大型网格文件时建议 8GB+）
- **存储**：至少 500MB 可用空间

### 软件要求
- **Windows**
  - Windows 10/11
  - Visual Studio 2019+（支持 C++17）
  - CMake 3.20+
  - VTK 9.5+
  - Qt 6.10+（仅用于 GUI 应用）

- **Linux**
  - 支持的 Linux 发行版（Ubuntu 20.04+, CentOS 8+）
  - GCC 7+ 或 Clang 6+
  - CMake 3.20+
  - VTK 9.5+
  - Qt 6.10+（仅用于 GUI 应用）

- **macOS**
  - macOS 10.15+
  - Xcode 11+
  - CMake 3.20+
  - VTK 9.5+
  - Qt 6.10+（仅用于 GUI 应用）

## 安装与配置

### 方法一：从源码构建

#### 1. 克隆项目

```bash
git clone https://github.com/yourusername/MeshFormatConverter.git
cd MeshFormatConverter
```

#### 2. 配置依赖

**Windows**

1. 下载并安装 VTK 9.5+：[VTK 官网](https://vtk.org/download/)
2. 下载并安装 Qt 6.10+：[Qt 官网](https://www.qt.io/download)
3. （可选）下载并安装 CGNS 4.0+：[CGNS 官网](https://cgns.github.io/)
4. （可选）下载并安装 Gmsh 4.1+：[Gmsh 官网](https://gmsh.info/)

**Linux**

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential cmake qtbase5-dev libvtk9-dev

# 可选依赖
sudo apt-get install libcgns-dev gmsh
```

**macOS**

```bash
# 使用 Homebrew
brew install cmake vtk qt

# 可选依赖
brew install cgns gmsh
```

#### 3. 构建项目

```bash
# 创建构建目录
mkdir build
cd build

# 配置项目（Windows）
cmake .. -DCMAKE_PREFIX_PATH="D:/Qt/6.10.2/msvc2022_64" -DVTK_DIR="D:/code/lib/VTK-9.5.2-install/lib/cmake/vtk-9.5"

# 配置项目（Linux/macOS）
cmake ..

# 构建项目
cmake --build . --config Release

# 安装项目（可选）
cmake --install .
```

### 方法二：使用预编译二进制文件

预编译的二进制文件将在后续版本中提供，敬请期待。

## 使用指南

### 命令行工具

#### meshconv 命令行工具

```bash
# 基本用法
meshconv <input_file> <output_file>

# 示例：将 STL 文件转换为 VTK 文件
meshconv input.stl output.vtk

# 示例：将 VTK 文件转换为 CGNS 文件
meshconv input.vtk output.cgns

# 示例：将 Gmsh 文件转换为 STL 文件
meshconv input.msh output.stl
```

### Qt GUI 应用程序

#### QtTransformApp 使用指南

1. **启动应用程序**

   ```bash
   # Windows
   bin/Release/QtTransformApp.exe
   
   # Linux
   bin/QtTransformApp
   
   # macOS
   bin/QtTransformApp.app/Contents/MacOS/QtTransformApp
   ```

2. **导入网格文件**

   - **方法一**：通过菜单栏的"文件" -> "打开文件"选项选择一个或多个网格文件
   - **方法二**：直接将网格文件拖放到文件浏览器中
   - **方法三**：双击文件浏览器中的网格文件

3. **查看网格信息**

   - **基本信息**：查看网格的文件名、类型、格式、大小、导入时间和维度
   - **单元统计**：查看单元类型和数量
   - **属性信息**：查看点属性、单元属性和物理区域

4. **导出网格文件**

   - 在文件浏览器或"已加载网格"标签页中选择一个网格文件
   - 在"导出配置"标签页中选择目标格式
   - 选择导出选项（体网格/面网格、二进制/ASCII）
   - 点击"导出"按钮执行转换操作
   - 在导出终端中查看导出进度和结果

5. **已加载网格管理**

   - **树形显示**：在"已加载网格"标签页中以树形结构查看所有已导入的网格
   - **右键菜单**：为网格节点提供"导出"、"移除"和"查看属性"快捷操作
   - **自动填充**：选择网格时自动填充导出格式和路径

### C++ API

#### 基本用法

```cpp
#include "MeshConverter.h"
#include "MeshHelper.h"

int main() {
    std::string srcPath = "input.vtk";
    std::string dstPath = "output.cgns";
    
    // 自动检测输入文件格式
    MeshFormat srcFormat = MeshHelper::detectFormat(srcPath);
    MeshFormat dstFormat = MeshFormat::CGNS;
    
    // 设置写入选项
    FormatWriteOptions writeOptions;
    writeOptions.isBinary = true;
    
    // 执行转换
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

#### 批量转换

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

## API 文档

### 核心类

| 类名 | 描述 | 主要方法 |
|------|------|----------|
| `MeshReader` | 网格读取模块 | `readAuto()`, `readVTK()`, `readCGNS()`, `readSTL()` |
| `MeshWriter` | 网格写入模块 | `writeVTK()`, `writeCGNS()`, `writeSTL()`, `writeOBJ()` |
| `MeshConverter` | 格式转换模块 | `convert()`, `batchConvert()` |
| `MeshProcessor` | 网格处理模块 | `extractSurface()`, `validateMesh()` |
| `MeshHelper` | 辅助接口模块 | `detectFormat()`, `extractMetadata()` |
| `VTKConverter` | VTK 格式转换模块 | `convertFromVTK()`, `convertToVTK()` |

### 数据结构

| 结构名 | 描述 | 主要成员 |
|--------|------|----------|
| `MeshData` | 网格核心数据 | `points`, `cells`, `metadata` |
| `MeshMetadata` | 网格元数据 | `cellTypeCount`, `pointDataNames`, `cellDataNames` |
| `FormatWriteOptions` | 格式写入选项 | `isBinary`, `cgnsBaseName`, `cgnsZoneName` |
| `MeshException` | 异常处理类 | `errorCode`, `errorMessage` |

### 枚举类型

| 枚举名 | 描述 | 主要值 |
|--------|------|----------|
| `MeshFormat` | 网格格式枚举 | `VTK_LEGACY`, `VTK_XML`, `CGNS`, `GMSH_V4`, `STL_ASCII`, `STL_BINARY`, `OBJ`, `PLY_ASCII`, `PLY_BINARY`, `OFF` |
| `MeshErrorCode` | 错误码枚举 | `SUCCESS`, `FILE_NOT_FOUND`, `FORMAT_NOT_SUPPORTED`, `READ_ERROR`, `WRITE_ERROR`, `MEMORY_ERROR` |
| `VtkCellType` | VTK 单元类型枚举 | `VERTEX`, `LINE`, `TRIANGLE`, `QUAD`, `TETRA`, `HEXAHEDRON`, `WEDGE`, `PYRAMID` |

## 贡献指南

我们欢迎社区贡献，无论是添加新功能、修复 bug 还是改进文档。

### 开发流程

1. **Fork 项目**：在 GitHub 上 fork 项目仓库
2. **创建分支**：创建一个新的分支用于你的贡献
3. **实现功能**：实现你的功能或修复
4. **编写测试**：为你的更改编写测试
5. **提交代码**：提交你的更改并推送至你的 fork
6. **创建 Pull Request**：创建一个 Pull Request 描述你的更改

### 代码风格

- 遵循 C++17 标准
- 使用 4 个空格进行缩进
- 类名使用 PascalCase
- 方法名和函数名使用 camelCase
- 变量名使用 camelCase
- 常量和枚举值使用 UPPER_SNAKE_CASE
- 成员变量使用 m_ 前缀
- 头文件使用包含守卫
- 提供清晰的注释

### 测试

- 为新功能编写测试
- 确保现有测试通过
- 运行集成测试验证整体功能

### 文档

- 为新功能更新文档
- 使用 Doxygen 风格的注释
- 更新 README.md 文件（如需要）

## 许可证

本项目采用 MIT 许可证，详情请参阅 [LICENSE](LICENSE) 文件。

## 联系方式

### 项目维护者

- 姓名：项目维护团队
- 邮箱：contact@meshformatconverter.com
- GitHub：[yourusername/MeshFormatConverter](https://github.com/yourusername/MeshFormatConverter)

### 问题反馈

如有问题或建议，请通过以下方式反馈：

1. **GitHub Issues**：在 GitHub 仓库中创建一个 Issue
2. **邮件**：发送邮件至 contact@meshformatconverter.com
3. **讨论区**：在 GitHub Discussions 中参与讨论

### 贡献者

感谢所有为项目做出贡献的开发者！

## 鸣谢

本项目的开发和维护离不开以下开源项目的支持：

- [VTK (Visualization Toolkit)](https://vtk.org/)：提供强大的网格处理和可视化功能
- [CGNS](https://cgns.github.io/)：提供 CFD 数据格式支持
- [Gmsh](https://gmsh.info/)：提供网格生成和处理功能
- [Qt](https://www.qt.io/)：提供跨平台 GUI 框架
- [CMake](https://cmake.org/)：提供跨平台构建系统

## 更新日志

### 版本 1.0.0 (2026-02-06)

- 初始发布
- 支持多种网格格式的相互转换
- 提供命令行工具和 Qt GUI 应用程序
- 实现网格信息查看和导出功能
- 支持批量转换操作
- 修复 STL 文件格式检测和内存分配问题
- 添加对结构化网格和曲线网格的支持

## 路线图

### 未来计划

- [ ] 添加更多网格格式支持（如 SU2、OpenFOAM 等）
- [ ] 实现网格质量分析功能
- [ ] 添加网格简化和细化功能
- [ ] 提供更多导出配置选项
- [ ] 优化大型网格文件的处理性能
- [ ] 增加更多测试用例和文档
- [ ] 提供预编译二进制文件

---

<p align="center">
  <strong>MeshFormatConverter - 让网格格式转换变得简单</strong>
</p>

<p align="center">
  <a href="https://github.com/yourusername/MeshFormatConverter">
    <img src="https://img.shields.io/github/stars/yourusername/MeshFormatConverter.svg?style=social&label=Star&maxAge=2592000" alt="GitHub stars">
  </a>
  <a href="https://github.com/yourusername/MeshFormatConverter/fork">
    <img src="https://img.shields.io/github/forks/yourusername/MeshFormatConverter.svg?style=social&label=Fork&maxAge=2592000" alt="GitHub forks">
  </a>
</p>