#include "MeshTypes.h"

/**
 * @brief 清空所有数据
 */
void MeshData::clear() {
    points.clear();
    cells.clear();
    pointData.clear();
    cellData.clear();
    metadata = MeshMetadata(); // 重置为默认值
}

/**
 * @brief 判断是否为空
 * @return 是否为空
 */
bool MeshData::isEmpty() const {
    return points.empty() && cells.empty();
}

/**
 * @brief 从几何/拓扑数据计算元数据
 */
void MeshData::calculateMetadata() {
    // 计算点数量
    metadata.pointCount = points.size() / 3;
    
    // 计算单元数量
    metadata.cellCount = cells.size();
    
    // 计算各单元类型数量
    metadata.cellTypeCount.clear();
    for (const auto& cell : cells) {
        metadata.cellTypeCount[cell.type]++;
    }
    
    // 提取点属性名称
    metadata.pointDataNames.clear();
    for (const auto& [name, _] : pointData) {
        metadata.pointDataNames.push_back(name);
    }
    
    // 提取单元属性名称
    metadata.cellDataNames.clear();
    for (const auto& [name, _] : cellData) {
        metadata.cellDataNames.push_back(name);
    }
    
    // 确定网格类型
    if (cells.empty()) {
        metadata.meshType = MeshType::UNKNOWN;
    } else {
        // 简单判断：如果包含体单元则为体网格，否则为面网格
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
