#pragma once

#include <string>
#include <cstdint>
#include "MeshTypes.h"
#include "MeshException.h"

/**
 * @brief 网格处理模块
 * 提供网格拓扑/几何处理能力，如体网格提取表面、面网格转体网格等
 */
class MeshProcessor {
public:
    /**
     * @brief 从体网格提取表面网格（生成闭合壳体）
     * @param volumeMesh 输入体网格数据
     * @param[out] surfaceMesh 输出表面网格数据
     * @param includeBoundaryOnly 是否仅提取边界单元（true=仅边界，false=所有表面）
     * @param[out] errorCode 输出错误码
     * @param[out] errorMsg 输出错误信息
     * @return 处理是否成功
     */
    static bool extractSurfaceFromVolume(const MeshData& volumeMesh,
                                        MeshData& surfaceMesh,
                                        bool includeBoundaryOnly,
                                        MeshErrorCode& errorCode,
                                        std::string& errorMsg);

    /**
     * @brief 网格数据校验（检查点/单元索引、属性数据长度等）
     * @param meshData 待校验的网格数据
     * @param[out] errorMsg 输出校验失败信息
     * @return 校验是否通过
     */
    static bool validateMesh(const MeshData& meshData, std::string& errorMsg);

    /**
     * @brief 计算网格边界盒
     * @param meshData 网格数据
     * @param[out] bounds 输出边界盒 [minX, maxX, minY, maxY, minZ, maxZ]
     * @return 计算是否成功
     */
    static bool computeBounds(const MeshData& meshData, std::vector<float>& bounds);

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
    static bool smoothMesh(const MeshData& meshData,
                          MeshData& smoothedMesh,
                          int iterations,
                          float relaxation,
                          MeshErrorCode& errorCode,
                          std::string& errorMsg);

    /**
     * @brief 网格简化
     * @param meshData 输入网格数据
     * @param[out] simplifiedMesh 输出简化后的网格数据
     * @param targetReduction 目标简化比例（0-1）
     * @param[out] errorCode 输出错误码
     * @param[out] errorMsg 输出错误信息
     * @return 处理是否成功
     */
    static bool simplifyMesh(const MeshData& meshData,
                            MeshData& simplifiedMesh,
                            float targetReduction,
                            MeshErrorCode& errorCode,
                            std::string& errorMsg);

private:
    /**
     * @brief 检查点索引是否有效
     * @param pointIndex 点索引
     * @param pointCount 点数量
     * @return 是否有效
     */
    static bool isValidPointIndex(uint32_t pointIndex, uint64_t pointCount);
};
