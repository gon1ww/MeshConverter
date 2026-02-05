#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

/**
 * @brief Mesh data type (volume/surface)
 */
enum class MeshType {
    UNKNOWN = 0,
    VOLUME_MESH = 1,  // Volume mesh (tetrahedron, hexahedron, etc.)
    SURFACE_MESH = 2  // Surface mesh (triangle, quadrilateral, etc.)
};

/**
 * @brief Supported mesh formats
 */
enum class MeshFormat {
    UNKNOWN = 0,
    // Volume mesh formats
    VTK_LEGACY = 1,    // VTK Legacy (.vtk)
    VTK_XML = 2,       // VTK XML (.vtu/.vtp/.vti/.vts)
    CGNS = 3,          // CGNS (.cgns)
    GMSH_V2 = 4,       // Gmsh v2 (.msh)
    GMSH_V4 = 5,       // Gmsh v4 (.msh)
    SU2 = 6,           // SU2 (.su2)
    OPENFOAM = 7,      // OpenFOAM (foamFile)
    // Surface mesh formats
    STL_ASCII = 8,     // STL ASCII (.stl)
    STL_BINARY = 9,    // STL Binary (.stl)
    OBJ = 10,          // OBJ (.obj)
    PLY_ASCII = 11,    // PLY ASCII (.ply)
    PLY_BINARY = 12,   // PLY Binary (.ply)
    OFF = 13           // OFF (.off)
};

/**
 * @brief VTK cell type (mapping VTK native definition)
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
    // Extended common types
    TRIANGLE_STRIP = 6,
    POLYGON = 7
};

/**
 * @brief Error code definition
 */
enum class MeshErrorCode {
    SUCCESS = 0,                // Success
    FILE_NOT_EXIST = 1,         // File not exist
    FORMAT_UNSUPPORTED = 2,     // Format not supported
    READ_FAILED = 3,            // Read failed (parse error/permission issue)
    WRITE_FAILED = 4,           // Write failed (permission issue/disk full)
    MESH_EMPTY = 5,             // Mesh data is empty
    PARAM_INVALID = 6,          // Invalid parameter
    DEPENDENCY_MISSING = 7,     // Dependency library missing (e.g. CGNS API not loaded)
    FORMAT_VERSION_INVALID = 8  // Format version incompatible (e.g. Gmsh v1 format)
};

/**
 * @brief Mesh metadata (describes mesh properties, does not include geometry/topology data)
 */
struct MeshMetadata {
    std::string fileName;                // Source file name
    MeshType meshType = MeshType::UNKNOWN; // Volume/surface mesh type
    MeshFormat format = MeshFormat::UNKNOWN; // Source format
    uint64_t pointCount = 0;             // Point count
    uint64_t cellCount = 0;             // Cell count
    std::unordered_map<VtkCellType, uint64_t> cellTypeCount; // Count of each cell type
    std::vector<std::string> physicalRegions; // Physical region names (e.g. CFD boundary conditions)
    std::vector<std::string> pointDataNames;  // Point attribute names (e.g. pressure, velocity)
    std::vector<std::string> cellDataNames;   // Cell attribute names (e.g. Jacobian, skewness)
    std::string formatVersion;           // Format version (e.g. VTK 4.2, Gmsh 4.1)
};

/**
 * @brief Format write options (format-specific configurations)
 */
struct FormatWriteOptions {
    // Common options
    bool isBinary = true;                // Whether to use binary storage (default true, prioritize performance)
    int precision = 6;                   // Floating point precision (valid for ASCII format)
    bool compress = false;               // Whether to compress (only supported by VTK XML/CGNS)
    // VTK-specific options
    bool vtkPreserveAllAttributes = true; // Whether to preserve all attribute data
    // CGNS-specific options
    std::string cgnsBaseName = "Base1";  // CGNS Base name
    std::string cgnsZoneName = "Zone1";  // CGNS Zone name
    int cgnsDimension = 3;               // CGNS physical dimension (2/3)
    // Gmsh-specific options
    bool gmshPreservePhysicalGroups = true; // Whether to preserve physical groups
    // STL-specific options
    std::string stlSolidName = "Solid";  // STL solid name (ASCII format)
};

/**
 * @brief Mesh core data (geometry + topology + attributes)
 */
class MeshData {
public:
    // Geometry data: point coordinates (x,y,z), stored contiguously
    std::vector<float> points; // Length = pointCount*3, index: i*3=x, i*3+1=y, i*3+2=z
    
    // Topology data: cell connectivity
    struct Cell {
        VtkCellType type;      // Cell type
        std::vector<uint32_t> pointIndices; // Point indices contained in the cell (starting from 0)
    };
    std::vector<Cell> cells;   // All cells
    
    // Attribute data: point attributes (name->value list)
    std::unordered_map<std::string, std::vector<float>> pointData;
    
    // Attribute data: cell attributes (name->value list)
    std::unordered_map<std::string, std::vector<float>> cellData;
    
    // Metadata
    MeshMetadata metadata;

    // Basic operations
    void clear();                          // Clear all data
    bool isEmpty() const;                  // Check if empty
    void calculateMetadata();              // Calculate metadata from geometry/topology data
};
