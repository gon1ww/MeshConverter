#include "MeshReader.h"
#include <iostream>
#include <string>

/**
 * @brief Convert MeshFormat to string
 * @param format Mesh format
 * @return String representation
 */
std::string formatToString(MeshFormat format) {
    switch (format) {
        case MeshFormat::VTK_LEGACY: return "VTK Legacy";
        case MeshFormat::VTK_XML: return "VTK XML";
        case MeshFormat::CGNS: return "CGNS";
        case MeshFormat::GMSH_V2: return "Gmsh v2";
        case MeshFormat::GMSH_V4: return "Gmsh v4";
        case MeshFormat::SU2: return "SU2";
        case MeshFormat::OPENFOAM: return "OpenFOAM";
        case MeshFormat::STL_ASCII: return "STL ASCII";
        case MeshFormat::STL_BINARY: return "STL Binary";
        case MeshFormat::OBJ: return "OBJ";
        case MeshFormat::PLY_ASCII: return "PLY ASCII";
        case MeshFormat::PLY_BINARY: return "PLY Binary";
        case MeshFormat::OFF: return "OFF";
        default: return "Unknown";
    }
}

/**
 * @brief Convert MeshType to string
 * @param type Mesh type
 * @return String representation
 */
std::string meshTypeToString(MeshType type) {
    switch (type) {
        case MeshType::VOLUME_MESH: return "Volume Mesh";
        case MeshType::SURFACE_MESH: return "Surface Mesh";
        default: return "Unknown";
    }
}

/**
 * @brief Convert VtkCellType to string
 * @param type Cell type
 * @return String representation
 */
std::string cellTypeToString(VtkCellType type) {
    switch (type) {
        case VtkCellType::VERTEX: return "Vertex";
        case VtkCellType::LINE: return "Line";
        case VtkCellType::TRIANGLE: return "Triangle";
        case VtkCellType::QUAD: return "Quad";
        case VtkCellType::TETRA: return "Tetrahedron";
        case VtkCellType::HEXAHEDRON: return "Hexahedron";
        case VtkCellType::WEDGE: return "Wedge";
        case VtkCellType::PYRAMID: return "Pyramid";
        case VtkCellType::TRIANGLE_STRIP: return "Triangle Strip";
        case VtkCellType::POLYGON: return "Polygon";
        default: return "Unknown";
    }
}

/**
 * @brief Convert MeshErrorCode to string
 * @param code Error code
 * @return String representation
 */
std::string errorCodeToString(MeshErrorCode code) {
    switch (code) {
        case MeshErrorCode::SUCCESS: return "Success";
        case MeshErrorCode::FILE_NOT_EXIST: return "File not exist";
        case MeshErrorCode::FORMAT_UNSUPPORTED: return "Format not supported";
        case MeshErrorCode::READ_FAILED: return "Read failed";
        case MeshErrorCode::WRITE_FAILED: return "Write failed";
        case MeshErrorCode::MESH_EMPTY: return "Mesh empty";
        case MeshErrorCode::PARAM_INVALID: return "Parameter invalid";
        case MeshErrorCode::DEPENDENCY_MISSING: return "Dependency missing";
        case MeshErrorCode::FORMAT_VERSION_INVALID: return "Format version invalid";
        default: return "Unknown error";
    }
}

/**
 * @brief Main function for MeshReaderTest
 * @param argc Argument count
 * @param argv Argument values
 * @return Exit code
 */
int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc != 2) {
        std::cerr << "Usage: MeshReaderTest <mesh_file_path>" << std::endl;
        std::cerr << "Example: MeshReaderTest D:/path/to/mesh.vtk" << std::endl;
        return 1;
    }

    std::string filePath = argv[1];
    std::cout << "MeshReaderTest - Reading mesh file: " << filePath << std::endl;
    std::cout << "====================================" << std::endl;

    // Detect file format
    MeshFormat format = MeshReader::detectFormatFromHeader(filePath);
    std::cout << "File format detected: " << formatToString(format) << std::endl;

    // Read mesh data
    MeshData meshData;
    MeshErrorCode errorCode;
    std::string errorMsg;

    std::cout << "\nReading mesh data..." << std::endl;
    bool success = MeshReader::readAuto(filePath, meshData, errorCode, errorMsg);

    if (success) {
        std::cout << "Yes, Read successful!" << std::endl;
        std::cout << "\nMesh information:" << std::endl;
        std::cout << "- Number of points: " << meshData.metadata.pointCount << std::endl;
        std::cout << "- Number of cells: " << meshData.metadata.cellCount << std::endl;
        std::cout << "- Mesh type: " << meshTypeToString(meshData.metadata.meshType) << std::endl;
        
        // Print cell type distribution
        if (!meshData.metadata.cellTypeCount.empty()) {
            std::cout << "\nCell type distribution:" << std::endl;
            for (const auto& [type, count] : meshData.metadata.cellTypeCount) {
                std::cout << "  - " << cellTypeToString(type) << ": " << count << std::endl;
            }
        }
        
        // Print attribute information
        if (!meshData.pointData.empty()) {
            std::cout << "\nPoint attributes:" << std::endl;
            for (const auto& [name, data] : meshData.pointData) {
                std::cout << "  - " << name << " (" << data.size() / meshData.metadata.pointCount << " components)" << std::endl;
            }
        }
        
        if (!meshData.cellData.empty()) {
            std::cout << "\nCell attributes:" << std::endl;
            for (const auto& [name, data] : meshData.cellData) {
                std::cout << "  - " << name << " (" << data.size() / meshData.metadata.cellCount << " components)" << std::endl;
                
                // Verify temperature and pressure attributes specifically
                if (name == "temperature" || name == "pressure") {
                    std::cout << "    Values: ";
                    size_t valuesToShow = std::min(static_cast<size_t>(5), data.size());
                    for (size_t i = 0; i < valuesToShow; ++i) {
                        std::cout << data[i];
                        if (i < valuesToShow - 1) {
                            std::cout << ", ";
                        }
                    }
                    if (data.size() > 5) {
                        std::cout << ", ...";
                    }
                    std::cout << std::endl;
                    
                    // Calculate number of components
                    int numComponents = data.size() / meshData.metadata.cellCount;
                    
                    // Verify data type and format
                    std::cout << "    Data type: float" << std::endl;
                    std::cout << "    Total values: " << data.size() << std::endl;
                    std::cout << "    Components per cell: " << numComponents << std::endl;
                    std::cout << "    Expected values: " << meshData.metadata.cellCount * numComponents << std::endl;
                    std::cout << "    Data length matches expected: " << (data.size() == meshData.metadata.cellCount * numComponents ? "Yes" : "No") << std::endl;
                }
            }
        }
        
        std::cout << "\n====================================" << std::endl;
        std::cout << "MeshReaderTest completed successfully!" << std::endl;
        return 0;
    } else {
        std::cerr << "No, Read failed!" << std::endl;
        std::cerr << "Error code: " << static_cast<int>(errorCode) << " (" << errorCodeToString(errorCode) << ")" << std::endl;
        std::cerr << "Error message: " << errorMsg << std::endl;
        std::cerr << "====================================" << std::endl;
        return 1;
    }
}
