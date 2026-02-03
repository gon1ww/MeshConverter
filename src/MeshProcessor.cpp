#include "MeshProcessor.h"
#include <algorithm>
#include <limits>

/**
 * @brief 检查点索引是否有效
 * @param pointIndex 点索引
 * @param pointCount 点数量
 * @return 是否有效
 */
bool MeshProcessor::isValidPointIndex(uint32_t pointIndex, uint64_t pointCount) {
    return pointIndex < pointCount;
}

/**
 * @brief 从体网格提取表面网格（生成闭合壳体）
 * @param volumeMesh 输入体网格数据
 * @param[out] surfaceMesh 输出表面网格数据
 * @param includeBoundaryOnly 是否仅提取边界单元（true=仅边界，false=所有表面）
 * @param[out] errorCode 输出错误码
 * @param[out] errorMsg 输出错误信息
 * @return 处理是否成功
 */
bool MeshProcessor::extractSurfaceFromVolume(const MeshData& volumeMesh,
                                           MeshData& surfaceMesh,
                                           bool includeBoundaryOnly,
                                           MeshErrorCode& errorCode,
                                           std::string& errorMsg) {
    // 检查输入网格是否为空
    if (volumeMesh.isEmpty()) {
        errorCode = MeshErrorCode::MESH_EMPTY;
        errorMsg = "输入网格数据为空";
        return false;
    }

    // 检查是否为体网格
    if (volumeMesh.metadata.meshType != MeshType::VOLUME_MESH) {
        errorCode = MeshErrorCode::PARAM_INVALID;
        errorMsg = "输入网格不是体网格";
        return false;
    }

    // 清空输出网格
    surfaceMesh.clear();

    // 这里实现体网格提取表面的逻辑
    // 基本思路：
    // 1. 遍历所有单元的面
    // 2. 统计每个面出现的次数
    // 3. 只保留出现次数为1的面（边界面）

    // 暂时返回未实现
    errorCode = MeshErrorCode::FORMAT_VERSION_INVALID;
    errorMsg = "体网格提取表面功能未实现";
    return false;
}

/**
 * @brief 网格数据校验（检查点/单元索引、属性数据长度等）
 * @param meshData 待校验的网格数据
 * @param[out] errorMsg 输出校验失败信息
 * @return 校验是否通过
 */
bool MeshProcessor::validateMesh(const MeshData& meshData, std::string& errorMsg) {
    // 检查网格是否为空
    if (meshData.isEmpty()) {
        errorMsg = "网格数据为空";
        return false;
    }

    // 检查点数据
    if (meshData.points.size() % 3 != 0) {
        errorMsg = "点数据长度不是3的倍数";
        return false;
    }

    uint64_t pointCount = meshData.points.size() / 3;

    // 检查单元数据
    for (size_t i = 0; i < meshData.cells.size(); i++) {
        const auto& cell = meshData.cells[i];
        for (uint32_t pointIndex : cell.pointIndices) {
            if (!isValidPointIndex(pointIndex, pointCount)) {
                errorMsg = "单元 " + std::to_string(i) + " 包含无效的点索引: " + std::to_string(pointIndex);
                return false;
            }
        }
    }

    // 检查点属性数据
    for (const auto& [name, data] : meshData.pointData) {
        if (!data.empty() && data.size() % pointCount != 0) {
            errorMsg = "点属性 '" + name + "' 数据长度与点数量不匹配";
            return false;
        }
    }

    // 检查单元属性数据
    uint64_t cellCount = meshData.cells.size();
    for (const auto& [name, data] : meshData.cellData) {
        if (!data.empty() && data.size() % cellCount != 0) {
            errorMsg = "单元属性 '" + name + "' 数据长度与单元数量不匹配";
            return false;
        }
    }

    return true;
}

/**
 * @brief 计算网格边界盒
 * @param meshData 网格数据
 * @param[out] bounds 输出边界盒 [minX, maxX, minY, maxY, minZ, maxZ]
 * @return 计算是否成功
 */
bool MeshProcessor::computeBounds(const MeshData& meshData, std::vector<float>& bounds) {
    // 检查网格是否为空
    if (meshData.isEmpty()) {
        return false;
    }

    // 初始化边界盒
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();

    // 遍历所有点
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

    // 填充输出
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
 * @brief 网格平滑处理
 * @param meshData 输入网格数据
 * @param[out] smoothedMesh 输出平滑后的网格数据
 * @param iterations 平滑迭代次数
 * @param relaxation 松弛因子（0-1）
 * @param[out] errorCode 输出错误码
 * @param[out] errorMsg 输出错误信息
 * @return 处理是否成功
 */
bool MeshProcessor::smoothMesh(const MeshData& meshData,
                              MeshData& smoothedMesh,
                              int iterations,
                              float relaxation,
                              MeshErrorCode& errorCode,
                              std::string& errorMsg) {
    // 检查输入参数
    if (iterations < 0) {
        errorCode = MeshErrorCode::PARAM_INVALID;
        errorMsg = "迭代次数不能为负数";
        return false;
    }

    if (relaxation < 0.0f || relaxation > 1.0f) {
        errorCode = MeshErrorCode::PARAM_INVALID;
        errorMsg = "松弛因子必须在0-1之间";
        return false;
    }

    // 检查输入网格是否为空
    if (meshData.isEmpty()) {
        errorCode = MeshErrorCode::MESH_EMPTY;
        errorMsg = "输入网格数据为空";
        return false;
    }

    // 这里实现网格平滑的逻辑
    // 基本思路：
    // 1. 对每个点，计算其邻接点的平均值
    // 2. 按照松弛因子移动点的位置
    // 3. 迭代指定次数

    // 暂时返回未实现
    errorCode = MeshErrorCode::FORMAT_VERSION_INVALID;
    errorMsg = "网格平滑功能未实现";
    return false;
}

/**
 * @brief 网格简化
 * @param meshData 输入网格数据
 * @param[out] simplifiedMesh 输出简化后的网格数据
 * @param targetReduction 目标简化比例（0-1）
 * @param[out] errorCode 输出错误码
 * @param[out] errorMsg 输出错误信息
 * @return 处理是否成功
 */
bool MeshProcessor::simplifyMesh(const MeshData& meshData,
                               MeshData& simplifiedMesh,
                               float targetReduction,
                               MeshErrorCode& errorCode,
                               std::string& errorMsg) {
    // 检查输入参数
    if (targetReduction < 0.0f || targetReduction >= 1.0f) {
        errorCode = MeshErrorCode::PARAM_INVALID;
        errorMsg = "目标简化比例必须在0-1之间";
        return false;
    }

    // 检查输入网格是否为空
    if (meshData.isEmpty()) {
        errorCode = MeshErrorCode::MESH_EMPTY;
        errorMsg = "输入网格数据为空";
        return false;
    }

    // 这里实现网格简化的逻辑
    // 基本思路：
    // 1. 使用边折叠算法
    // 2. 计算边的折叠代价
    // 3. 按照代价从小到大折叠边
    // 4. 直到达到目标简化比例

    // 暂时返回未实现
    errorCode = MeshErrorCode::FORMAT_VERSION_INVALID;
    errorMsg = "网格简化功能未实现";
    return false;
}
