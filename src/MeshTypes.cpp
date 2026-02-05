#include "MeshTypes.h"

/**
 * @brief Clear all data
 */
void MeshData::clear() {
    points.clear();
    cells.clear();
    pointData.clear();
    cellData.clear();
    metadata = MeshMetadata(); // Reset to default values
}

/**
 * @brief Check if mesh is empty
 * @return Whether empty
 */
bool MeshData::isEmpty() const {
    return points.empty() && cells.empty();
}

/**
 * @brief Calculate metadata from geometry/topology data
 */
void MeshData::calculateMetadata() {
    // Calculate point count
    metadata.pointCount = points.size() / 3;
    
    // Calculate cell count
    metadata.cellCount = cells.size();
    
    // Calculate count of each cell type
    metadata.cellTypeCount.clear();
    for (const auto& cell : cells) {
        metadata.cellTypeCount[cell.type]++;
    }
    
    // Extract point attribute names
    metadata.pointDataNames.clear();
    for (const auto& [name, _] : pointData) {
        metadata.pointDataNames.push_back(name);
    }
    
    // Extract cell attribute names
    metadata.cellDataNames.clear();
    for (const auto& [name, _] : cellData) {
        metadata.cellDataNames.push_back(name);
    }
    
    // Determine mesh type
    if (cells.empty()) {
        metadata.meshType = MeshType::UNKNOWN;
    } else {
        // Simple判断：如果包含体单元则为体网格，否则为面网格
        bool hasVolumeCells = false;
        for (const auto& cell : cells) {
            switch (cell.type) {
                case VtkCellType::TETRA:
                case VtkCellType::HEXAHEDRON:
                case VtkCellType::WEDGE:
                case VtkCellType::PYRAMID:
                    hasVolumeCells = true;
                    break;
                default:
                    break;
            }
        }
        metadata.meshType = hasVolumeCells ? MeshType::VOLUME_MESH : MeshType::SURFACE_MESH;
    }
}
