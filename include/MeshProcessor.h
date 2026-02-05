#pragma once

#include <string>
#include <cstdint>
#include "MeshTypes.h"
#include "MeshException.h"

/**
 * @brief Mesh processing module
 * Provides mesh topology/geometry processing capabilities, such as extracting surface from volume mesh, converting surface mesh to volume mesh, etc.
 */
class MeshProcessor {
public:
    /**
     * @brief Extract surface mesh from volume mesh (generate closed shell)
     * @param volumeMesh Input volume mesh data
     * @param[out] surfaceMesh Output surface mesh data
     * @param includeBoundaryOnly Whether to extract only boundary cells (true=only boundary, false=all surfaces)
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message
     * @return Whether processing is successful
     */
    static bool extractSurfaceFromVolume(const MeshData& volumeMesh,
                                        MeshData& surfaceMesh,
                                        bool includeBoundaryOnly,
                                        MeshErrorCode& errorCode,
                                        std::string& errorMsg);

    /**
     * @brief Mesh data validation (check point/cell indices, attribute data length, etc.)
     * @param meshData Mesh data to validate
     * @param[out] errorMsg Output validation failure message
     * @return Whether validation passes
     */
    static bool validateMesh(const MeshData& meshData, std::string& errorMsg);

    /**
     * @brief Calculate mesh bounding box
     * @param meshData Mesh data
     * @param[out] bounds Output bounding box [minX, maxX, minY, maxY, minZ, maxZ]
     * @return Whether calculation is successful
     */
    static bool computeBounds(const MeshData& meshData, std::vector<float>& bounds);

    /**
     * @brief Mesh smoothing
     * @param meshData Input mesh data
     * @param[out] smoothedMesh Output smoothed mesh data
     * @param iterations Smoothing iteration count
     * @param relaxation Relaxation factor (0-1)
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message
     * @return Whether processing is successful
     */
    static bool smoothMesh(const MeshData& meshData,
                          MeshData& smoothedMesh,
                          int iterations,
                          float relaxation,
                          MeshErrorCode& errorCode,
                          std::string& errorMsg);

    /**
     * @brief Mesh simplification
     * @param meshData Input mesh data
     * @param[out] simplifiedMesh Output simplified mesh data
     * @param targetReduction Target simplification ratio (0-1)
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message
     * @return Whether processing is successful
     */
    static bool simplifyMesh(const MeshData& meshData,
                            MeshData& simplifiedMesh,
                            float targetReduction,
                            MeshErrorCode& errorCode,
                            std::string& errorMsg);

private:
    /**
     * @brief Check if point index is valid
     * @param pointIndex Point index
     * @param pointCount Point count
     * @return Whether valid
     */
    static bool isValidPointIndex(uint32_t pointIndex, uint64_t pointCount);
};
