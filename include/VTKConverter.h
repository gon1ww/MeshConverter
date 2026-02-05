#pragma once

#include <string>
#include <vtkUnstructuredGrid.h>
#include "MeshTypes.h"
#include "MeshException.h"

/**
 * @brief VTKConverter class
 * Responsible for VTK-based 3D model format conversion workflow
 * Supports source format → VTK → processing → target format conversion
 */
class VTKConverter {
public:
    /**
     * @brief VTK processing options
     */
    struct VTKProcessingOptions {
        bool enableCleaning = true;          // Enable duplicate point removal
        bool enableTriangulation = false;     // Enable triangulation of polygons
        bool enableDecimation = false;        // Enable mesh decimation
        double decimationTarget = 0.5;        // Target reduction factor (0.0-1.0)
        bool enableSmoothing = false;         // Enable mesh smoothing
        int smoothingIterations = 20;         // Number of smoothing iterations
        double smoothingRelaxation = 0.1;     // Smoothing relaxation factor
        bool enableNormalComputation = false; // Enable normal vector computation
        bool preserveTopology = true;         // Preserve topology during processing
    };

    /**
     * @brief Convert source format file to VTK format
     * @param srcFilePath Source file path
     * @param vtkGrid Output VTK unstructured grid
     * @param errorCode Output error code
     * @param errorMsg Output error message
     * @return Whether conversion is successful
     */
    static bool convertToVTK(const std::string& srcFilePath, 
                             vtkSmartPointer<vtkUnstructuredGrid>& vtkGrid, 
                             MeshErrorCode& errorCode, 
                             std::string& errorMsg);

    /**
     * @brief Process and optimize VTK data
     * @param inputGrid Input VTK unstructured grid
     * @param options Processing options
     * @param outputGrid Output processed VTK unstructured grid
     * @param errorCode Output error code
     * @param errorMsg Output error message
     * @return Whether processing is successful
     */
    static bool processVTKData(const vtkSmartPointer<vtkUnstructuredGrid>& inputGrid, 
                               const VTKProcessingOptions& options, 
                               vtkSmartPointer<vtkUnstructuredGrid>& outputGrid, 
                               MeshErrorCode& errorCode, 
                               std::string& errorMsg);

    /**
     * @brief Convert VTK data to target format
     * @param vtkGrid Input VTK unstructured grid
     * @param dstFilePath Destination file path
     * @param dstFormat Target format
     * @param writeOptions Write options
     * @param errorCode Output error code
     * @param errorMsg Output error message
     * @return Whether conversion is successful
     */
    static bool convertFromVTK(const vtkSmartPointer<vtkUnstructuredGrid>& vtkGrid, 
                               const std::string& dstFilePath, 
                               MeshFormat dstFormat, 
                               const FormatWriteOptions& writeOptions, 
                               MeshErrorCode& errorCode, 
                               std::string& errorMsg);

    /**
     * @brief Complete conversion workflow: source → VTK → processing → target
     * @param srcFilePath Source file path
     * @param dstFilePath Destination file path
     * @param srcFormat Source format
     * @param dstFormat Target format
     * @param processingOptions VTK processing options
     * @param writeOptions Write options
     * @param errorCode Output error code
     * @param errorMsg Output error message
     * @return Whether conversion is successful
     */
    static bool convert(const std::string& srcFilePath, 
                       const std::string& dstFilePath, 
                       MeshFormat srcFormat, 
                       MeshFormat dstFormat, 
                       const VTKProcessingOptions& processingOptions, 
                       const FormatWriteOptions& writeOptions, 
                       MeshErrorCode& errorCode, 
                       std::string& errorMsg);

private:
    /**
     * @brief Convert vtkUnstructuredGrid to MeshData
     * @param grid Input VTK unstructured grid
     * @param[out] meshData Output mesh data
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message
     * @return Whether conversion is successful
     */
    static bool vtkToMeshData(const vtkSmartPointer<vtkUnstructuredGrid>& grid, 
                              MeshData& meshData, 
                              MeshErrorCode& errorCode, 
                              std::string& errorMsg);
};
