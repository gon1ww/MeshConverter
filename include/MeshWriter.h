#pragma once

#include <string>
#include <vtkUnstructuredGrid.h>
#include "MeshTypes.h"
#include "MeshException.h"

/**
 * @brief Mesh writer module
 * Responsible for writing MeshData to files in specified formats, supporting format-specific configurations
 */
class MeshWriter {
public:
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
    static bool write(const MeshData& meshData,
                      const std::string& filePath,
                      MeshFormat targetFormat,
                      const FormatWriteOptions& options,
                      MeshErrorCode& errorCode,
                      std::string& errorMsg);

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
    static bool writeVTK(const MeshData& meshData,
                         const std::string& filePath,
                         bool isXml,
                         const FormatWriteOptions& options,
                         MeshErrorCode& errorCode,
                         std::string& errorMsg);

    /**
     * @brief Write CGNS format file
     * @param meshData Input mesh data
     * @param filePath Output file path (UTF-8 encoded)
     * @param options Write options (need to specify cgnsBaseName/cgnsZoneName etc.)
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message
     * @return Whether writing is successful
     */
    static bool writeCGNS(const MeshData& meshData,
                          const std::string& filePath,
                          const FormatWriteOptions& options,
                          MeshErrorCode& errorCode,
                          std::string& errorMsg);

    /**
     * @brief Write Gmsh format file
     * @param meshData Input mesh data
     * @param filePath Output file path (UTF-8 encoded)
     * @param options Write options
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message
     * @return Whether writing is successful
     */
    static bool writeGmsh(const MeshData& meshData,
                          const std::string& filePath,
                          const FormatWriteOptions& options,
                          MeshErrorCode& errorCode,
                          std::string& errorMsg);

    /**
     * @brief Write STL format file
     * @param meshData Input mesh data
     * @param filePath Output file path (UTF-8 encoded)
     * @param options Write options
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message
     * @return Whether writing is successful
     */
    static bool writeSTL(const MeshData& meshData,
                         const std::string& filePath,
                         const FormatWriteOptions& options,
                         MeshErrorCode& errorCode,
                         std::string& errorMsg);

    /**
     * @brief Write OBJ format file
     * @param meshData Input mesh data
     * @param filePath Output file path (UTF-8 encoded)
     * @param options Write options
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message
     * @return Whether writing is successful
     */
    static bool writeOBJ(const MeshData& meshData,
                        const std::string& filePath,
                        const FormatWriteOptions& options,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg);

    /**
     * @brief Write PLY format file
     * @param meshData Input mesh data
     * @param filePath Output file path (UTF-8 encoded)
     * @param options Write options
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message
     * @return Whether writing is successful
     */
    static bool writePLY(const MeshData& meshData,
                        const std::string& filePath,
                        const FormatWriteOptions& options,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg);

    /**
     * @brief Write OFF format file
     * @param meshData Input mesh data
     * @param filePath Output file path (UTF-8 encoded)
     * @param options Write options
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message
     * @return Whether writing is successful
     */
    static bool writeOFF(const MeshData& meshData,
                        const std::string& filePath,
                        const FormatWriteOptions& options,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg);

    /**
     * @brief Write SU2 format file
     * @param meshData Input mesh data
     * @param filePath Output file path (UTF-8 encoded)
     * @param options Write options
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message
     * @return Whether writing is successful
     */
    static bool writeSU2(const MeshData& meshData,
                        const std::string& filePath,
                        const FormatWriteOptions& options,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg);

    /**
     * @brief Write OpenFOAM format file
     * @param meshData Input mesh data
     * @param filePath Output file path (UTF-8 encoded)
     * @param options Write options
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message
     * @return Whether writing is successful
     */
    static bool writeOpenFOAM(const MeshData& meshData,
                             const std::string& filePath,
                             const FormatWriteOptions& options,
                             MeshErrorCode& errorCode,
                             std::string& errorMsg);

    // --------------------------------------------------------------------------
    // VTK intermediate format related methods
    // --------------------------------------------------------------------------

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
    static bool writeVTK(const vtkSmartPointer<vtkUnstructuredGrid>& grid,
                        const std::string& filePath,
                        MeshFormat targetFormat,
                        const FormatWriteOptions& options,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg);

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
    static bool writeVTKToVTK(const vtkSmartPointer<vtkUnstructuredGrid>& grid,
                              const std::string& filePath,
                              bool isXml,
                              const FormatWriteOptions& options,
                              MeshErrorCode& errorCode,
                              std::string& errorMsg);

    /**
     * @brief Write vtkUnstructuredGrid to CGNS format file
     * @param grid Input vtkUnstructuredGrid
     * @param filePath Output file path (UTF-8 encoded)
     * @param options Write options (need to specify cgnsBaseName/cgnsZoneName etc.)
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message
     * @return Whether writing is successful
     */
    static bool writeVTKToCGNS(const vtkSmartPointer<vtkUnstructuredGrid>& grid,
                               const std::string& filePath,
                               const FormatWriteOptions& options,
                               MeshErrorCode& errorCode,
                               std::string& errorMsg);

    /**
     * @brief Write vtkUnstructuredGrid to Gmsh format file
     * @param grid Input vtkUnstructuredGrid
     * @param filePath Output file path (UTF-8 encoded)
     * @param options Write options
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message
     * @return Whether writing is successful
     */
    static bool writeVTKToGmsh(const vtkSmartPointer<vtkUnstructuredGrid>& grid,
                               const std::string& filePath,
                               const FormatWriteOptions& options,
                               MeshErrorCode& errorCode,
                               std::string& errorMsg);

    /**
     * @brief Write vtkUnstructuredGrid to STL format file
     * @param grid Input vtkUnstructuredGrid
     * @param filePath Output file path (UTF-8 encoded)
     * @param options Write options
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message
     * @return Whether writing is successful
     */
    static bool writeVTKToSTL(const vtkSmartPointer<vtkUnstructuredGrid>& grid,
                              const std::string& filePath,
                              const FormatWriteOptions& options,
                              MeshErrorCode& errorCode,
                              std::string& errorMsg);

    /**
     * @brief Write vtkUnstructuredGrid to OBJ format file
     * @param grid Input vtkUnstructuredGrid
     * @param filePath Output file path (UTF-8 encoded)
     * @param options Write options
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message
     * @return Whether writing is successful
     */
    static bool writeVTKToOBJ(const vtkSmartPointer<vtkUnstructuredGrid>& grid,
                              const std::string& filePath,
                              const FormatWriteOptions& options,
                              MeshErrorCode& errorCode,
                              std::string& errorMsg);

    /**
     * @brief Write vtkUnstructuredGrid to PLY format file
     * @param grid Input vtkUnstructuredGrid
     * @param filePath Output file path (UTF-8 encoded)
     * @param options Write options
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message
     * @return Whether writing is successful
     */
    static bool writeVTKToPLY(const vtkSmartPointer<vtkUnstructuredGrid>& grid,
                              const std::string& filePath,
                              const FormatWriteOptions& options,
                              MeshErrorCode& errorCode,
                              std::string& errorMsg);

    /**
     * @brief Write vtkUnstructuredGrid to OFF format file
     * @param grid Input vtkUnstructuredGrid
     * @param filePath Output file path (UTF-8 encoded)
     * @param options Write options
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message
     * @return Whether writing is successful
     */
    static bool writeVTKToOFF(const vtkSmartPointer<vtkUnstructuredGrid>& grid,
                              const std::string& filePath,
                              const FormatWriteOptions& options,
                              MeshErrorCode& errorCode,
                              std::string& errorMsg);

    /**
     * @brief Write vtkUnstructuredGrid to SU2 format file
     * @param grid Input vtkUnstructuredGrid
     * @param filePath Output file path (UTF-8 encoded)
     * @param options Write options
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message
     * @return Whether writing is successful
     */
    static bool writeVTKToSU2(const vtkSmartPointer<vtkUnstructuredGrid>& grid,
                              const std::string& filePath,
                              const FormatWriteOptions& options,
                              MeshErrorCode& errorCode,
                              std::string& errorMsg);

    /**
     * @brief Write vtkUnstructuredGrid to OpenFOAM format file
     * @param grid Input vtkUnstructuredGrid
     * @param filePath Output file path (UTF-8 encoded)
     * @param options Write options
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message
     * @return Whether writing is successful
     */
    static bool writeVTKToOpenFOAM(const vtkSmartPointer<vtkUnstructuredGrid>& grid,
                                   const std::string& filePath,
                                   const FormatWriteOptions& options,
                                   MeshErrorCode& errorCode,
                                   std::string& errorMsg);

private:
    /**
     * @brief Ensure directory exists
     * @param filePath File path
     * @return Whether successful
     */
    static bool ensureDirectoryExists(const std::string& filePath);

    /**
     * @brief Check if mesh data is empty
     * @param meshData Mesh data
     * @return Whether empty
     */
    static bool isMeshEmpty(const MeshData& meshData);

    /**
     * @brief Convert vtkUnstructuredGrid to MeshData
     * @param grid Input vtkUnstructuredGrid
     * @param[out] meshData Output mesh data
     * @return Whether successful
     */
    static bool vtkToMeshData(const vtkSmartPointer<vtkUnstructuredGrid>& grid,
                             MeshData& meshData);
};
