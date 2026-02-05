#include "MeshWriter.h"
#include <fstream>
#include <filesystem>

/**
 * @brief Ensure directory exists
 * @param filePath File path
 * @return Whether successful
 */
bool MeshWriter::ensureDirectoryExists(const std::string& filePath) {
    std::filesystem::path path(filePath);
    std::filesystem::path dir = path.parent_path();
    
    if (dir.empty()) {
        return true; // Directory is empty, no need to create
    }
    
    if (!std::filesystem::exists(dir)) {
        try {
            std::filesystem::create_directories(dir);
            return true;
        } catch (...) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Check if mesh data is empty
 * @param meshData Mesh data
 * @return Whether empty
 */
bool MeshWriter::isMeshEmpty(const MeshData& meshData) {
    return meshData.isEmpty();
}

/**
 * @brief Write mesh data to specified format file
 * @param meshData Input mesh data
 * @param filePath Output file path (UTF-8 encoded)
 * @param targetFormat Target format
 * @param options Write options (format-specific configurations)
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message (UTF-8)
 * @return Whether writing is successful
 */
bool MeshWriter::write(const MeshData& meshData,
                      const std::string& filePath,
                      MeshFormat targetFormat,
                      const FormatWriteOptions& options,
                      MeshErrorCode& errorCode,
                      std::string& errorMsg) {
    // Check if mesh data is empty
    if (isMeshEmpty(meshData)) {
        errorCode = MeshErrorCode::MESH_EMPTY;
        errorMsg = "Mesh data is empty";
        return false;
    }

    // Ensure directory exists
    if (!ensureDirectoryExists(filePath)) {
        errorCode = MeshErrorCode::WRITE_FAILED;
        errorMsg = "Cannot create output directory";
        return false;
    }

    // Call corresponding write method based on format
    switch (targetFormat) {
        case MeshFormat::VTK_LEGACY:
            return writeVTK(meshData, filePath, false, options, errorCode, errorMsg);
        case MeshFormat::VTK_XML:
            return writeVTK(meshData, filePath, true, options, errorCode, errorMsg);
        case MeshFormat::CGNS:
            return writeCGNS(meshData, filePath, options, errorCode, errorMsg);
        case MeshFormat::GMSH_V2:
        case MeshFormat::GMSH_V4:
            return writeGmsh(meshData, filePath, options, errorCode, errorMsg);
        case MeshFormat::STL_ASCII:
        case MeshFormat::STL_BINARY:
            return writeSTL(meshData, filePath, options, errorCode, errorMsg);
        case MeshFormat::OBJ:
            return writeOBJ(meshData, filePath, options, errorCode, errorMsg);
        case MeshFormat::PLY_ASCII:
        case MeshFormat::PLY_BINARY:
            return writePLY(meshData, filePath, options, errorCode, errorMsg);
        case MeshFormat::OFF:
            return writeOFF(meshData, filePath, options, errorCode, errorMsg);
        case MeshFormat::SU2:
            return writeSU2(meshData, filePath, options, errorCode, errorMsg);
        case MeshFormat::OPENFOAM:
            return writeOpenFOAM(meshData, filePath, options, errorCode, errorMsg);
        default:
            errorCode = MeshErrorCode::FORMAT_UNSUPPORTED;
            errorMsg = "Format not supported";
            return false;
    }
}

/**
 * @brief Write VTK format file (automatically distinguish Legacy/XML)
 * @param meshData Input mesh data
 * @param filePath Output file path (UTF-8 encoded)
 * @param isXml Whether to write XML format (false=Legacy, true=XML)
 * @param options Write options
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message
 * @return Whether writing is successful
 */
bool MeshWriter::writeVTK(const MeshData& meshData,
                         const std::string& filePath,
                         bool isXml,
                         const FormatWriteOptions& options,
                         MeshErrorCode& errorCode,
                         std::string& errorMsg) {
    // Use VTK library to implement specific write logic
    // Temporarily return unimplemented
    errorCode = MeshErrorCode::FORMAT_VERSION_INVALID;
    errorMsg = "VTK format write not implemented";
    return false;
}

/**
 * @brief Write CGNS format file
 * @param meshData Input mesh data
 * @param filePath Output file path (UTF-8 encoded)
 * @param options Write options (need to specify cgnsBaseName/cgnsZoneName, etc.)
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message
 * @return Whether writing is successful
 */
bool MeshWriter::writeCGNS(const MeshData& meshData,
                          const std::string& filePath,
                          const FormatWriteOptions& options,
                          MeshErrorCode& errorCode,
                          std::string& errorMsg) {
    // Check if there is CGNS dependency
#ifndef HAVE_CGNS
    errorCode = MeshErrorCode::DEPENDENCY_MISSING;
    errorMsg = "CGNS dependency library missing";
    return false;
#endif

    // Use CGNS library to implement specific write logic
    // Temporarily return unimplemented
    errorCode = MeshErrorCode::FORMAT_VERSION_INVALID;
    errorMsg = "CGNS format write not implemented";
    return false;
}

/**
 * @brief Write Gmsh format file
 * @param meshData Input mesh data
 * @param filePath Output file path (UTF-8 encoded)
 * @param options Write options
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message
 * @return Whether writing is successful
 */
bool MeshWriter::writeGmsh(const MeshData& meshData,
                          const std::string& filePath,
                          const FormatWriteOptions& options,
                          MeshErrorCode& errorCode,
                          std::string& errorMsg) {
    // Check if there is Gmsh dependency
#ifndef HAVE_GMSH
    errorCode = MeshErrorCode::DEPENDENCY_MISSING;
    errorMsg = "Gmsh dependency library missing";
    return false;
#endif

    // Use Gmsh library to implement specific write logic
    // Temporarily return unimplemented
    errorCode = MeshErrorCode::FORMAT_VERSION_INVALID;
    errorMsg = "Gmsh format write not implemented";
    return false;
}

/**
 * @brief Write STL format file
 * @param meshData Input mesh data
 * @param filePath Output file path (UTF-8 encoded)
 * @param options Write options
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message
 * @return Whether writing is successful
 */
bool MeshWriter::writeSTL(const MeshData& meshData,
                         const std::string& filePath,
                         const FormatWriteOptions& options,
                         MeshErrorCode& errorCode,
                         std::string& errorMsg) {
    // Implement STL format write logic here
    // Temporarily return unimplemented
    errorCode = MeshErrorCode::FORMAT_VERSION_INVALID;
    errorMsg = "STL format write not implemented";
    return false;
}

/**
 * @brief Write OBJ format file
 * @param meshData Input mesh data
 * @param filePath Output file path (UTF-8 encoded)
 * @param options Write options
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message
 * @return Whether writing is successful
 */
bool MeshWriter::writeOBJ(const MeshData& meshData,
                        const std::string& filePath,
                        const FormatWriteOptions& options,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg) {
    // Implement OBJ format write logic here
    // Temporarily return unimplemented
    errorCode = MeshErrorCode::FORMAT_VERSION_INVALID;
    errorMsg = "OBJ format write not implemented";
    return false;
}

/**
 * @brief Write PLY format file
 * @param meshData Input mesh data
 * @param filePath Output file path (UTF-8 encoded)
 * @param options Write options
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message
 * @return Whether writing is successful
 */
bool MeshWriter::writePLY(const MeshData& meshData,
                        const std::string& filePath,
                        const FormatWriteOptions& options,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg) {
    // Implement PLY format write logic here
    // Temporarily return unimplemented
    errorCode = MeshErrorCode::FORMAT_VERSION_INVALID;
    errorMsg = "PLY format write not implemented";
    return false;
}

/**
 * @brief Write OFF format file
 * @param meshData Input mesh data
 * @param filePath Output file path (UTF-8 encoded)
 * @param options Write options
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message
 * @return Whether writing is successful
 */
bool MeshWriter::writeOFF(const MeshData& meshData,
                        const std::string& filePath,
                        const FormatWriteOptions& options,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg) {
    // Implement OFF format write logic here
    // Temporarily return unimplemented
    errorCode = MeshErrorCode::FORMAT_VERSION_INVALID;
    errorMsg = "OFF format write not implemented";
    return false;
}

/**
 * @brief Write SU2 format file
 * @param meshData Input mesh data
 * @param filePath Output file path (UTF-8 encoded)
 * @param options Write options
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message
 * @return Whether writing is successful
 */
bool MeshWriter::writeSU2(const MeshData& meshData,
                        const std::string& filePath,
                        const FormatWriteOptions& options,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg) {
    // Implement SU2 format write logic here
    // Temporarily return unimplemented
    errorCode = MeshErrorCode::FORMAT_VERSION_INVALID;
    errorMsg = "SU2 format write not implemented";
    return false;
}

/**
 * @brief Write OpenFOAM format file
 * @param meshData Input mesh data
 * @param filePath Output file path (UTF-8 encoded)
 * @param options Write options
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message
 * @return Whether writing is successful
 */
bool MeshWriter::writeOpenFOAM(const MeshData& meshData,
                             const std::string& filePath,
                             const FormatWriteOptions& options,
                             MeshErrorCode& errorCode,
                             std::string& errorMsg) {
    // Implement OpenFOAM format write logic here
    // Temporarily return unimplemented
    errorCode = MeshErrorCode::FORMAT_VERSION_INVALID;
    errorMsg = "OpenFOAM format write not implemented";
    return false;
}

// --------------------------------------------------------------------------
// VTK intermediate format related method implementations
// --------------------------------------------------------------------------

/**
 * @brief Convert vtkUnstructuredGrid to MeshData
 * @param grid Input vtkUnstructuredGrid
 * @param[out] meshData Output mesh data
 * @return Whether successful
 */
bool MeshWriter::vtkToMeshData(const vtkSmartPointer<vtkUnstructuredGrid>& grid,
                             MeshData& meshData) {
    // Clear existing data
    meshData.points.clear();
    meshData.cells.clear();
    meshData.pointData.clear();
    meshData.cellData.clear();
    
    // Reset metadata
    meshData.metadata = MeshMetadata();
    
    // Get point set
    vtkPoints* points = grid->GetPoints();
    if (!points) {
        return false;
    }
    
    // Add points
    vtkIdType numPoints = points->GetNumberOfPoints();
    for (vtkIdType i = 0; i < numPoints; ++i) {
        double coords[3];
        points->GetPoint(i, coords);
        
        meshData.points.push_back(static_cast<float>(coords[0]));
        meshData.points.push_back(static_cast<float>(coords[1]));
        meshData.points.push_back(static_cast<float>(coords[2]));
    }
    
    // Add cells
    vtkIdType numCells = grid->GetNumberOfCells();
    for (vtkIdType i = 0; i < numCells; ++i) {
        vtkCell* cell = grid->GetCell(i);
        if (!cell) {
            continue;
        }
        
        MeshData::Cell newCell;
        
        // Determine cell type
        int cellType = cell->GetCellType();
        switch (cellType) {
        case VTK_TETRA:
            newCell.type = VtkCellType::TETRA;
            break;
        case VTK_HEXAHEDRON:
            newCell.type = VtkCellType::HEXAHEDRON;
            break;
        case VTK_WEDGE:
            newCell.type = VtkCellType::WEDGE;
            break;
        case VTK_PYRAMID:
            newCell.type = VtkCellType::PYRAMID;
            break;
        case VTK_TRIANGLE:
            newCell.type = VtkCellType::TRIANGLE;
            break;
        case VTK_QUAD:
            newCell.type = VtkCellType::QUAD;
            break;
        case VTK_LINE:
            newCell.type = VtkCellType::LINE;
            break;
        case VTK_VERTEX:
            newCell.type = VtkCellType::VERTEX;
            break;
        case VTK_TRIANGLE_STRIP:
            newCell.type = VtkCellType::TRIANGLE_STRIP;
            break;
        case VTK_POLYGON:
            newCell.type = VtkCellType::POLYGON;
            break;
        default:
            continue;
        }
        
        // Add node IDs
        vtkIdType numCellPoints = cell->GetNumberOfPoints();
        for (vtkIdType j = 0; j < numCellPoints; ++j) {
            newCell.pointIndices.push_back(static_cast<uint32_t>(cell->GetPointId(j)));
        }
        
        meshData.cells.push_back(newCell);
    }
    
    // Calculate metadata
    meshData.calculateMetadata();
    
    return true;
}

/**
 * @brief Write vtkUnstructuredGrid to specified format file
 * @param grid Input vtkUnstructuredGrid
 * @param filePath Output file path (UTF-8 encoded)
 * @param targetFormat Target format
 * @param options Write options (format-specific configurations)
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message (UTF-8)
 * @return Whether writing is successful
 */
bool MeshWriter::writeVTK(const vtkSmartPointer<vtkUnstructuredGrid>& grid,
                        const std::string& filePath,
                        MeshFormat targetFormat,
                        const FormatWriteOptions& options,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg) {
    // First convert vtkUnstructuredGrid to MeshData
    MeshData meshData;
    bool convertSuccess = vtkToMeshData(grid, meshData);
    if (!convertSuccess) {
        errorCode = MeshErrorCode::FORMAT_UNSUPPORTED;
        errorMsg = "Cannot convert vtkUnstructuredGrid to MeshData";
        return false;
    }
    
    // Use existing write method to write to specified format
    return write(meshData, filePath, targetFormat, options, errorCode, errorMsg);
}

/**
 * @brief Write vtkUnstructuredGrid to VTK format file
 * @param grid Input vtkUnstructuredGrid
 * @param filePath Output file path (UTF-8 encoded)
 * @param isXml Whether to write XML format (false=Legacy, true=XML)
 * @param options Write options
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message
 * @return Whether writing is successful
 */
bool MeshWriter::writeVTKToVTK(const vtkSmartPointer<vtkUnstructuredGrid>& grid,
                              const std::string& filePath,
                              bool isXml,
                              const FormatWriteOptions& options,
                              MeshErrorCode& errorCode,
                              std::string& errorMsg) {
    // First convert vtkUnstructuredGrid to MeshData
    MeshData meshData;
    bool convertSuccess = vtkToMeshData(grid, meshData);
    if (!convertSuccess) {
        errorCode = MeshErrorCode::FORMAT_UNSUPPORTED;
        errorMsg = "Cannot convert vtkUnstructuredGrid to MeshData";
        return false;
    }
    
    // Use existing writeVTK method to write
    return writeVTK(meshData, filePath, isXml, options, errorCode, errorMsg);
}

/**
 * @brief Write vtkUnstructuredGrid to CGNS format file
 * @param grid Input vtkUnstructuredGrid
 * @param filePath Output file path (UTF-8 encoded)
 * @param options Write options (need to specify cgnsBaseName/cgnsZoneName, etc.)
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message
 * @return Whether writing is successful
 */
bool MeshWriter::writeVTKToCGNS(const vtkSmartPointer<vtkUnstructuredGrid>& grid,
                               const std::string& filePath,
                               const FormatWriteOptions& options,
                               MeshErrorCode& errorCode,
                               std::string& errorMsg) {
    // First convert vtkUnstructuredGrid to MeshData
    MeshData meshData;
    bool convertSuccess = vtkToMeshData(grid, meshData);
    if (!convertSuccess) {
        errorCode = MeshErrorCode::FORMAT_UNSUPPORTED;
        errorMsg = "Cannot convert vtkUnstructuredGrid to MeshData";
        return false;
    }
    
    // Use existing writeCGNS method to write
    return writeCGNS(meshData, filePath, options, errorCode, errorMsg);
}

/**
 * @brief Write vtkUnstructuredGrid to Gmsh format file
 * @param grid Input vtkUnstructuredGrid
 * @param filePath Output file path (UTF-8 encoded)
 * @param options Write options
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message
 * @return Whether writing is successful
 */
bool MeshWriter::writeVTKToGmsh(const vtkSmartPointer<vtkUnstructuredGrid>& grid,
                               const std::string& filePath,
                               const FormatWriteOptions& options,
                               MeshErrorCode& errorCode,
                               std::string& errorMsg) {
    // First convert vtkUnstructuredGrid to MeshData
    MeshData meshData;
    bool convertSuccess = vtkToMeshData(grid, meshData);
    if (!convertSuccess) {
        errorCode = MeshErrorCode::FORMAT_UNSUPPORTED;
        errorMsg = "Cannot convert vtkUnstructuredGrid to MeshData";
        return false;
    }
    
    // Use existing writeGmsh method to write
    return writeGmsh(meshData, filePath, options, errorCode, errorMsg);
}

/**
 * @brief Write vtkUnstructuredGrid to STL format file
 * @param grid Input vtkUnstructuredGrid
 * @param filePath Output file path (UTF-8 encoded)
 * @param options Write options
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message
 * @return Whether writing is successful
 */
bool MeshWriter::writeVTKToSTL(const vtkSmartPointer<vtkUnstructuredGrid>& grid,
                              const std::string& filePath,
                              const FormatWriteOptions& options,
                              MeshErrorCode& errorCode,
                              std::string& errorMsg) {
    // First convert vtkUnstructuredGrid to MeshData
    MeshData meshData;
    bool convertSuccess = vtkToMeshData(grid, meshData);
    if (!convertSuccess) {
        errorCode = MeshErrorCode::FORMAT_UNSUPPORTED;
        errorMsg = "Cannot convert vtkUnstructuredGrid to MeshData";
        return false;
    }
    
    // Use existing writeSTL method to write
    return writeSTL(meshData, filePath, options, errorCode, errorMsg);
}

/**
 * @brief Write vtkUnstructuredGrid to OBJ format file
 * @param grid Input vtkUnstructuredGrid
 * @param filePath Output file path (UTF-8 encoded)
 * @param options Write options
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message
 * @return Whether writing is successful
 */
bool MeshWriter::writeVTKToOBJ(const vtkSmartPointer<vtkUnstructuredGrid>& grid,
                              const std::string& filePath,
                              const FormatWriteOptions& options,
                              MeshErrorCode& errorCode,
                              std::string& errorMsg) {
    // First convert vtkUnstructuredGrid to MeshData
    MeshData meshData;
    bool convertSuccess = vtkToMeshData(grid, meshData);
    if (!convertSuccess) {
        errorCode = MeshErrorCode::FORMAT_UNSUPPORTED;
        errorMsg = "Cannot convert vtkUnstructuredGrid to MeshData";
        return false;
    }
    
    // Use existing writeOBJ method to write
    return writeOBJ(meshData, filePath, options, errorCode, errorMsg);
}

/**
 * @brief Write vtkUnstructuredGrid to PLY format file
 * @param grid Input vtkUnstructuredGrid
 * @param filePath Output file path (UTF-8 encoded)
 * @param options Write options
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message
 * @return Whether writing is successful
 */
bool MeshWriter::writeVTKToPLY(const vtkSmartPointer<vtkUnstructuredGrid>& grid,
                              const std::string& filePath,
                              const FormatWriteOptions& options,
                              MeshErrorCode& errorCode,
                              std::string& errorMsg) {
    // First convert vtkUnstructuredGrid to MeshData
    MeshData meshData;
    bool convertSuccess = vtkToMeshData(grid, meshData);
    if (!convertSuccess) {
        errorCode = MeshErrorCode::FORMAT_UNSUPPORTED;
        errorMsg = "Cannot convert vtkUnstructuredGrid to MeshData";
        return false;
    }
    
    // Use existing writePLY method to write
    return writePLY(meshData, filePath, options, errorCode, errorMsg);
}

/**
 * @brief Write vtkUnstructuredGrid to OFF format file
 * @param grid Input vtkUnstructuredGrid
 * @param filePath Output file path (UTF-8 encoded)
 * @param options Write options
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message
 * @return Whether writing is successful
 */
bool MeshWriter::writeVTKToOFF(const vtkSmartPointer<vtkUnstructuredGrid>& grid,
                              const std::string& filePath,
                              const FormatWriteOptions& options,
                              MeshErrorCode& errorCode,
                              std::string& errorMsg) {
    // First convert vtkUnstructuredGrid to MeshData
    MeshData meshData;
    bool convertSuccess = vtkToMeshData(grid, meshData);
    if (!convertSuccess) {
        errorCode = MeshErrorCode::FORMAT_UNSUPPORTED;
        errorMsg = "Cannot convert vtkUnstructuredGrid to MeshData";
        return false;
    }
    
    // Use existing writeOFF method to write
    return writeOFF(meshData, filePath, options, errorCode, errorMsg);
}

/**
 * @brief Write vtkUnstructuredGrid to SU2 format file
 * @param grid Input vtkUnstructuredGrid
 * @param filePath Output file path (UTF-8 encoded)
 * @param options Write options
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message
 * @return Whether writing is successful
 */
bool MeshWriter::writeVTKToSU2(const vtkSmartPointer<vtkUnstructuredGrid>& grid,
                              const std::string& filePath,
                              const FormatWriteOptions& options,
                              MeshErrorCode& errorCode,
                              std::string& errorMsg) {
    // First convert vtkUnstructuredGrid to MeshData
    MeshData meshData;
    bool convertSuccess = vtkToMeshData(grid, meshData);
    if (!convertSuccess) {
        errorCode = MeshErrorCode::FORMAT_UNSUPPORTED;
        errorMsg = "Cannot convert vtkUnstructuredGrid to MeshData";
        return false;
    }
    
    // Use existing writeSU2 method to write
    return writeSU2(meshData, filePath, options, errorCode, errorMsg);
}

/**
 * @brief Write vtkUnstructuredGrid to OpenFOAM format file
 * @param grid Input vtkUnstructuredGrid
 * @param filePath Output file path (UTF-8 encoded)
 * @param options Write options
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message
 * @return Whether writing is successful
 */
bool MeshWriter::writeVTKToOpenFOAM(const vtkSmartPointer<vtkUnstructuredGrid>& grid,
                                   const std::string& filePath,
                                   const FormatWriteOptions& options,
                                   MeshErrorCode& errorCode,
                                   std::string& errorMsg) {
    // First convert vtkUnstructuredGrid to MeshData
    MeshData meshData;
    bool convertSuccess = vtkToMeshData(grid, meshData);
    if (!convertSuccess) {
        errorCode = MeshErrorCode::FORMAT_UNSUPPORTED;
        errorMsg = "Cannot convert vtkUnstructuredGrid to MeshData";
        return false;
    }
    
    // Use existing writeOpenFOAM method to write
    return writeOpenFOAM(meshData, filePath, options, errorCode, errorMsg);
}
