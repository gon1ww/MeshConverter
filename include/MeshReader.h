#pragma once

#include <string>
#include <vtkUnstructuredGrid.h>
#include "MeshTypes.h"
#include "MeshException.h"

/**
 * @brief Mesh reader module
 * Responsible for reading mesh data from files to MeshData, supporting automatic format detection and specified format reading
 */
class MeshReader {
public:
    /**
     * @brief Automatically detect file format and read mesh data
     * @param filePath File path (UTF-8 encoded)
     * @param[out] meshData Output mesh data
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message (UTF-8)
     * @return Whether reading is successful (true=success, false=failure)
     */
    static bool readAuto(const std::string& filePath,
                         MeshData& meshData,
                         MeshErrorCode& errorCode,
                         std::string& errorMsg);

    /**
     * @brief Read VTK format file (supports Legacy/XML automatic detection)
     * @param filePath File path (UTF-8 encoded)
     * @param[out] meshData Output mesh data
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message (UTF-8)
     * @return Whether reading is successful
     */
    static bool readVTK(const std::string& filePath,
                        MeshData& meshData,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg);

    /**
     * @brief Read CGNS format file
     * @param filePath File path (UTF-8 encoded)
     * @param[out] meshData Output mesh data
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message (UTF-8)
     * @param baseIndex CGNS Base index (default 0, first Base)
     * @param zoneIndex CGNS Zone index (default 0, first Zone)
     * @return Whether reading is successful
     */
    static bool readCGNS(const std::string& filePath,
                         MeshData& meshData,
                         MeshErrorCode& errorCode,
                         std::string& errorMsg,
                         int baseIndex = 0,
                         int zoneIndex = 0);

    /**
     * @brief Read Gmsh format file (supports v2/v4 automatic detection)
     * @param filePath File path (UTF-8 encoded)
     * @param[out] meshData Output mesh data
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message (UTF-8)
     * @return Whether reading is successful
     */
    static bool readGmsh(const std::string& filePath,
                         MeshData& meshData,
                         MeshErrorCode& errorCode,
                         std::string& errorMsg);

    /**
     * @brief Read STL format file (ASCII/Binary)
     * @param filePath File path (UTF-8 encoded)
     * @param[out] meshData Output mesh data
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message (UTF-8)
     * @return Whether reading is successful
     */
    static bool readSTL(const std::string& filePath,
                        MeshData& meshData,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg);

    /**
     * @brief Read OBJ format file
     * @param filePath File path (UTF-8 encoded)
     * @param[out] meshData Output mesh data
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message (UTF-8)
     * @return Whether reading is successful
     */
    static bool readOBJ(const std::string& filePath,
                        MeshData& meshData,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg);

    /**
     * @brief Read PLY format file (ASCII/Binary)
     * @param filePath File path (UTF-8 encoded)
     * @param[out] meshData Output mesh data
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message (UTF-8)
     * @return Whether reading is successful
     */
    static bool readPLY(const std::string& filePath,
                        MeshData& meshData,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg);

    /**
     * @brief Read OFF format file
     * @param filePath File path (UTF-8 encoded)
     * @param[out] meshData Output mesh data
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message (UTF-8)
     * @return Whether reading is successful
     */
    static bool readOFF(const std::string& filePath,
                        MeshData& meshData,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg);

    /**
     * @brief Read SU2 format file
     * @param filePath File path (UTF-8 encoded)
     * @param[out] meshData Output mesh data
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message (UTF-8)
     * @return Whether reading is successful
     */
    static bool readSU2(const std::string& filePath,
                        MeshData& meshData,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg);

    /**
     * @brief Read OpenFOAM format file
     * @param filePath File path (UTF-8 encoded)
     * @param[out] meshData Output mesh data
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message (UTF-8)
     * @return Whether reading is successful
     */
    static bool readOpenFOAM(const std::string& filePath,
                             MeshData& meshData,
                             MeshErrorCode& errorCode,
                             std::string& errorMsg);

    /**
     * @brief Detect format from file header
     * @param filePath File path
     * @return Detected format
     */
    static MeshFormat detectFormatFromHeader(const std::string& filePath);

    // --------------------------------------------------------------------------
    // VTK intermediate format related methods
    // --------------------------------------------------------------------------

    /**
     * @brief Automatically detect file format and read as vtkUnstructuredGrid
     * @param filePath File path (UTF-8 encoded)
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message (UTF-8)
     * @return vtkUnstructuredGrid pointer, returns nullptr on failure
     */
    static vtkSmartPointer<vtkUnstructuredGrid> readAutoToVTK(const std::string& filePath,
                                                              MeshErrorCode& errorCode,
                                                              std::string& errorMsg);

    /**
     * @brief Read VTK format file as vtkUnstructuredGrid
     * @param filePath File path (UTF-8 encoded)
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message (UTF-8)
     * @return vtkUnstructuredGrid pointer, returns nullptr on failure
     */
    static vtkSmartPointer<vtkUnstructuredGrid> readVTKToVTK(const std::string& filePath,
                                                             MeshErrorCode& errorCode,
                                                             std::string& errorMsg);

    /**
     * @brief Read CGNS format file as vtkUnstructuredGrid
     * @param filePath File path (UTF-8 encoded)
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message (UTF-8)
     * @param baseIndex CGNS Base index (default 0, first Base)
     * @param zoneIndex CGNS Zone index (default 0, first Zone)
     * @return vtkUnstructuredGrid pointer, returns nullptr on failure
     */
    static vtkSmartPointer<vtkUnstructuredGrid> readCGNSToVTK(const std::string& filePath,
                                                              MeshErrorCode& errorCode,
                                                              std::string& errorMsg,
                                                              int baseIndex = 0,
                                                              int zoneIndex = 0);

    /**
     * @brief Read Gmsh format file as vtkUnstructuredGrid
     * @param filePath File path (UTF-8 encoded)
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message (UTF-8)
     * @return vtkUnstructuredGrid pointer, returns nullptr on failure
     */
    static vtkSmartPointer<vtkUnstructuredGrid> readGmshToVTK(const std::string& filePath,
                                                              MeshErrorCode& errorCode,
                                                              std::string& errorMsg);

    /**
     * @brief Read STL format file as vtkUnstructuredGrid
     * @param filePath File path (UTF-8 encoded)
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message (UTF-8)
     * @return vtkUnstructuredGrid pointer, returns nullptr on failure
     */
    static vtkSmartPointer<vtkUnstructuredGrid> readSTLToVTK(const std::string& filePath,
                                                             MeshErrorCode& errorCode,
                                                             std::string& errorMsg);

    /**
     * @brief Read OBJ format file as vtkUnstructuredGrid
     * @param filePath File path (UTF-8 encoded)
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message (UTF-8)
     * @return vtkUnstructuredGrid pointer, returns nullptr on failure
     */
    static vtkSmartPointer<vtkUnstructuredGrid> readOBJToVTK(const std::string& filePath,
                                                             MeshErrorCode& errorCode,
                                                             std::string& errorMsg);

    /**
     * @brief Read PLY format file as vtkUnstructuredGrid
     * @param filePath File path (UTF-8 encoded)
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message (UTF-8)
     * @return vtkUnstructuredGrid pointer, returns nullptr on failure
     */
    static vtkSmartPointer<vtkUnstructuredGrid> readPLYToVTK(const std::string& filePath,
                                                             MeshErrorCode& errorCode,
                                                             std::string& errorMsg);

    /**
     * @brief Read OFF format file as vtkUnstructuredGrid
     * @param filePath File path (UTF-8 encoded)
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message (UTF-8)
     * @return vtkUnstructuredGrid pointer, returns nullptr on failure
     */
    static vtkSmartPointer<vtkUnstructuredGrid> readOFFToVTK(const std::string& filePath,
                                                             MeshErrorCode& errorCode,
                                                             std::string& errorMsg);

    /**
     * @brief Read SU2 format file as vtkUnstructuredGrid
     * @param filePath File path (UTF-8 encoded)
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message (UTF-8)
     * @return vtkUnstructuredGrid pointer, returns nullptr on failure
     */
    static vtkSmartPointer<vtkUnstructuredGrid> readSU2ToVTK(const std::string& filePath,
                                                             MeshErrorCode& errorCode,
                                                             std::string& errorMsg);

    /**
     * @brief Read OpenFOAM format file as vtkUnstructuredGrid
     * @param filePath File path (UTF-8 encoded)
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message (UTF-8)
     * @return vtkUnstructuredGrid pointer, returns nullptr on failure
     */
    static vtkSmartPointer<vtkUnstructuredGrid> readOpenFOAMToVTK(const std::string& filePath,
                                                                  MeshErrorCode& errorCode,
                                                                  std::string& errorMsg);

private:
    /**
     * @brief Check if file exists
     * @param filePath File path
     * @return Whether exists
     */
    static bool fileExists(const std::string& filePath);

    /**
     * @brief Read ASCII STL format
     * @param file Input file stream
     * @param[out] meshData Output mesh data
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message (UTF-8)
     * @return Whether reading is successful
     */
    static bool readSTLASCII(std::ifstream& file,
                             MeshData& meshData,
                             MeshErrorCode& errorCode,
                             std::string& errorMsg);

    /**
     * @brief Read Binary STL format
     * @param file Input file stream
     * @param[out] meshData Output mesh data
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message (UTF-8)
     * @return Whether reading is successful
     */
    static bool readSTLBinary(std::ifstream& file,
                              MeshData& meshData,
                              MeshErrorCode& errorCode,
                              std::string& errorMsg);

    /**
     * @brief Convert MeshData to vtkUnstructuredGrid
     * @param meshData Input mesh data
     * @return vtkUnstructuredGrid pointer
     */
    static vtkSmartPointer<vtkUnstructuredGrid> meshDataToVTK(const MeshData& meshData);
};
