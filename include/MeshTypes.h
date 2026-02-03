#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

/**
 * @brief 网格数据类型（体/面）
 */
enum class MeshType {
    UNKNOWN = 0,
    VOLUME_MESH = 1,  // 体网格（四面体、六面体等）
    SURFACE_MESH = 2  // 面网格（三角形、四边形等）
};

/**
 * @brief 支持的网格格式
 */
enum class MeshFormat {
    UNKNOWN = 0,
    // 体网格格式
    VTK_LEGACY = 1,    // VTK Legacy (.vtk)
    VTK_XML = 2,       // VTK XML (.vtu/.vtp/.vti/.vts)
    CGNS = 3,          // CGNS (.cgns)
    GMSH_V2 = 4,       // Gmsh v2 (.msh)
    GMSH_V4 = 5,       // Gmsh v4 (.msh)
    SU2 = 6,           // SU2 (.su2)
    OPENFOAM = 7,      // OpenFOAM (foamFile)
    // 面网格格式
    STL_ASCII = 8,     // STL ASCII (.stl)
    STL_BINARY = 9,    // STL Binary (.stl)
    OBJ = 10,          // OBJ (.obj)
    PLY_ASCII = 11,    // PLY ASCII (.ply)
    PLY_BINARY = 12,   // PLY Binary (.ply)
    OFF = 13           // OFF (.off)
};

/**
 * @brief VTK单元类型（映射VTK原生定义）
 */
enum class VtkCellType {
    VERTEX = 1,
    LINE = 3,
    TRIANGLE = 5,
    QUAD = 9,
    TETRA = 10,
    HEXAHEDRON = 12,
    WEDGE = 13,
    PYRAMID = 14,
    // 扩展常用类型
    TRIANGLE_STRIP = 6,
    POLYGON = 7
};

/**
 * @brief 错误码定义
 */
enum class MeshErrorCode {
    SUCCESS = 0,                // 成功
    FILE_NOT_EXIST = 1,         // 文件不存在
    FORMAT_UNSUPPORTED = 2,     // 格式不支持
    READ_FAILED = 3,            // 读取失败（解析错误/权限问题）
    WRITE_FAILED = 4,           // 写入失败（权限问题/磁盘满）
    MESH_EMPTY = 5,             // 网格数据为空
    PARAM_INVALID = 6,          // 参数无效
    DEPENDENCY_MISSING = 7,     // 依赖库缺失（如CGNS API未加载）
    FORMAT_VERSION_INVALID = 8  // 格式版本不兼容（如Gmsh v1格式）
};

/**
 * @brief 网格元数据（描述网格属性，不包含几何/拓扑数据）
 */
struct MeshMetadata {
    std::string fileName;                // 源文件名
    MeshType meshType = MeshType::UNKNOWN; // 体/面网格类型
    MeshFormat format = MeshFormat::UNKNOWN; // 源格式
    uint64_t pointCount = 0;             // 点数量
    uint64_t cellCount = 0;             // 单元数量
    std::unordered_map<VtkCellType, uint64_t> cellTypeCount; // 各单元类型数量
    std::vector<std::string> physicalRegions; // 物理区域名称（如CFD边界条件）
    std::vector<std::string> pointDataNames;  // 点属性名称（如压力、速度）
    std::vector<std::string> cellDataNames;   // 单元属性名称（如雅可比、扭曲度）
    std::string formatVersion;           // 格式版本（如VTK 4.2、Gmsh 4.1）
};

/**
 * @brief 格式写入选项（不同格式的特异性配置）
 */
struct FormatWriteOptions {
    // 通用选项
    bool isBinary = true;                // 是否二进制存储（默认true，优先高性能）
    int precision = 6;                   // 浮点数精度（ASCII格式有效）
    bool compress = false;               // 是否压缩（仅支持VTK XML/CGNS）
    // VTK专属选项
    bool vtkPreserveAllAttributes = true; // 是否保留所有属性数据
    // CGNS专属选项
    std::string cgnsBaseName = "Base1";  // CGNS Base名称
    std::string cgnsZoneName = "Zone1";  // CGNS Zone名称
    int cgnsDimension = 3;               // CGNS物理维度（2/3）
    // Gmsh专属选项
    bool gmshPreservePhysicalGroups = true; // 是否保留物理组
    // STL专属选项
    std::string stlSolidName = "Solid";  // STL实体名称（ASCII格式）
};

/**
 * @brief 网格核心数据（几何+拓扑+属性）
 */
class MeshData {
public:
    // 几何数据：点坐标（x,y,z），连续存储
    std::vector<float> points; // 长度=pointCount*3，索引：i*3=x, i*3+1=y, i*3+2=z
    
    // 拓扑数据：单元连接关系
    struct Cell {
        VtkCellType type;      // 单元类型
        std::vector<uint32_t> pointIndices; // 单元包含的点索引（从0开始）
    };
    std::vector<Cell> cells;   // 所有单元
    
    // 属性数据：点属性（名称->值列表）
    std::unordered_map<std::string, std::vector<float>> pointData;
    
    // 属性数据：单元属性（名称->值列表）
    std::unordered_map<std::string, std::vector<float>> cellData;
    
    // 元数据
    MeshMetadata metadata;

    // 基础操作
    void clear();                          // 清空所有数据
    bool isEmpty() const;                  // 判断是否为空
    void calculateMetadata();              // 从几何/拓扑数据计算元数据
};
