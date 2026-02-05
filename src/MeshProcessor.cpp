#include "MeshProcessor.h"
#include <algorithm>
#include <limits>

/**
 * @brief Check if point index is valid
 * @param pointIndex Point index
 * @param pointCount Point count
 * @return Whether valid
 */
bool MeshProcessor::isValidPointIndex(uint32_t pointIndex, uint64_t pointCount) {
    return pointIndex < pointCount;
}

/**
 * @brief Extract surface mesh from volume mesh (generate closed shell)
 * @param volumeMesh Input volume mesh data
 * @param[out] surfaceMesh Output surface mesh data
 * @param includeBoundaryOnly Whether to extract only boundary cells (true=only boundary, false=all surfaces)
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message
 * @return Whether processing is successful
 */
bool MeshProcessor::extractSurfaceFromVolume(const MeshData& volumeMesh,
                                           MeshData& surfaceMesh,
                                           bool includeBoundaryOnly,
                                           MeshErrorCode& errorCode,
                                           std::string& errorMsg) {
    // Check if input mesh is empty
    if (volumeMesh.isEmpty()) {
        errorCode = MeshErrorCode::MESH_EMPTY;
        errorMsg = "Input mesh data is empty";
        return false;
    }

    // Check if it is volume mesh
    if (volumeMesh.metadata.meshType != MeshType::VOLUME_MESH) {
        errorCode = MeshErrorCode::PARAM_INVALID;
        errorMsg = "Input mesh is not volume mesh";
        return false;
    }

    // Clear output mesh
    surfaceMesh.clear();

    // Implementation of surface extraction from volume mesh
    // Basic idea:
    // 1. Iterate through all faces of cells
    // 2. Count occurrence of each face
    // 3. Only keep faces with occurrence count 1 (boundary faces)

    // Temporarily return unimplemented
    errorCode = MeshErrorCode::FORMAT_VERSION_INVALID;
    errorMsg = "Surface extraction from volume mesh is not implemented";
    return false;
}

/**
 * @brief Mesh data validation (check point/cell indices, attribute data lengths, etc.)
 * @param meshData Mesh data to validate
 * @param[out] errorMsg Output validation failure message
 * @return Whether validation passes
 */
bool MeshProcessor::validateMesh(const MeshData& meshData, std::string& errorMsg) {
    // Check if mesh is empty
    if (meshData.isEmpty()) {
        errorMsg = "Mesh data is empty";
        return false;
    }

    // Check point data
    if (meshData.points.size() % 3 != 0) {
        errorMsg = "Point data length is not a multiple of 3";
        return false;
    }

    uint64_t pointCount = meshData.points.size() / 3;

    // Check cell data
    for (size_t i = 0; i < meshData.cells.size(); i++) {
        const auto& cell = meshData.cells[i];
        for (uint32_t pointIndex : cell.pointIndices) {
            if (!isValidPointIndex(pointIndex, pointCount)) {
                errorMsg = "Cell " + std::to_string(i) + " contains invalid point index: " + std::to_string(pointIndex);
                return false;
            }
        }
    }

    // Check point attribute data
    for (const auto& [name, data] : meshData.pointData) {
        if (!data.empty() && data.size() % pointCount != 0) {
            errorMsg = "Point attribute '" + name + "' data length does not match point count";
            return false;
        }
    }

    // Check cell attribute data
    uint64_t cellCount = meshData.cells.size();
    for (const auto& [name, data] : meshData.cellData) {
        if (!data.empty() && data.size() % cellCount != 0) {
            errorMsg = "Cell attribute '" + name + "' data length does not match cell count";
            return false;
        }
    }

    return true;
}

/**
 * @brief Compute mesh bounding box
 * @param meshData Mesh data
 * @param[out] bounds Output bounding box [minX, maxX, minY, maxY, minZ, maxZ]
 * @return Whether computation is successful
 */
bool MeshProcessor::computeBounds(const MeshData& meshData, std::vector<float>& bounds) {
    // Check if mesh is empty
    if (meshData.isEmpty()) {
        return false;
    }

    // Initialize bounding box
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();

    // Iterate through all points
    for (size_t i = 0; i < meshData.points.size(); i += 3) {
        float x = meshData.points[i];
        float y = meshData.points[i + 1];
        float z = meshData.points[i + 2];

        minX = std::min(minX, x);
        maxX = std::max(maxX, x);
        minY = std::min(minY, y);
        maxY = std::max(maxY, y);
        minZ = std::min(minZ, z);
        maxZ = std::max(maxZ, z);
    }

    // Fill output
    bounds.clear();
    bounds.push_back(minX);
    bounds.push_back(maxX);
    bounds.push_back(minY);
    bounds.push_back(maxY);
    bounds.push_back(minZ);
    bounds.push_back(maxZ);

    return true;
}

/**
 * @brief Mesh smoothing
 * @param meshData Input mesh data
 * @param[out] smoothedMesh Output smoothed mesh data
 * @param iterations Smoothing iterations
 * @param relaxation Relaxation factor (0-1)
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message
 * @return Whether processing is successful
 */
bool MeshProcessor::smoothMesh(const MeshData& meshData,
                              MeshData& smoothedMesh,
                              int iterations,
                              float relaxation,
                              MeshErrorCode& errorCode,
                              std::string& errorMsg) {
    // Check input parameters
    if (iterations < 0) {
        errorCode = MeshErrorCode::PARAM_INVALID;
        errorMsg = "Iteration count cannot be negative";
        return false;
    }

    if (relaxation < 0.0f || relaxation > 1.0f) {
        errorCode = MeshErrorCode::PARAM_INVALID;
        errorMsg = "Relaxation factor must be between 0-1";
        return false;
    }

    // Check if input mesh is empty
    if (meshData.isEmpty()) {
        errorCode = MeshErrorCode::MESH_EMPTY;
        errorMsg = "Input mesh data is empty";
        return false;
    }

    // Implementation of mesh smoothing
    // Basic idea:
    // 1. For each point, compute average of its neighboring points
    // 2. Move point position according to relaxation factor
    // 3. Iterate for specified number of times

    // Temporarily return unimplemented
    errorCode = MeshErrorCode::FORMAT_VERSION_INVALID;
    errorMsg = "Mesh smoothing is not implemented";
    return false;
}

/**
 * @brief Mesh simplification
 * @param meshData Input mesh data
 * @param[out] simplifiedMesh Output simplified mesh data
 * @param targetReduction Target reduction ratio (0-1)
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message
 * @return Whether processing is successful
 */
bool MeshProcessor::simplifyMesh(const MeshData& meshData,
                               MeshData& simplifiedMesh,
                               float targetReduction,
                               MeshErrorCode& errorCode,
                               std::string& errorMsg) {
    // Check input parameters
    if (targetReduction < 0.0f || targetReduction >= 1.0f) {
        errorCode = MeshErrorCode::PARAM_INVALID;
        errorMsg = "Target reduction ratio must be between 0-1";
        return false;
    }

    // Check if input mesh is empty
    if (meshData.isEmpty()) {
        errorCode = MeshErrorCode::MESH_EMPTY;
        errorMsg = "Input mesh data is empty";
        return false;
    }

    // Implementation of mesh simplification
    // Basic idea:
    // 1. Use edge collapse algorithm
    // 2. Compute collapse cost for edges
    // 3. Collapse edges in order of increasing cost
    // 4. Until target reduction ratio is reached

    // Temporarily return unimplemented
    errorCode = MeshErrorCode::FORMAT_VERSION_INVALID;
    errorMsg = "Mesh simplification is not implemented";
    return false;
}
