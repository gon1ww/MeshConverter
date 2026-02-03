#pragma once

#include <string>
#include "MeshTypes.h"
#include "MeshException.h"

/**
 * @brief 辅助接口模块
 * 提供格式识别、元数据提取等辅助能力
 */
class MeshHelper {
public:
    /**
     * @brief 从文件头识别网格格式
     * @param filePath 文件路径（UTF-8）
     * @return 识别出的格式（MeshFormat::UNKNOWN=无法识别）
     */
    static MeshFormat detectFormat(const std::string& filePath);

    /**
     * @brief 提取网格元数据（不加载完整几何/拓扑数据，提升性能）
     * @param filePath 文件路径（UTF-8）
     * @param[out] metadata 输出元数据
     * @param[out] errorCode 输出错误码
     * @param[out] errorMsg 输出错误信息
     * @return 提取是否成功
     */
    static bool extractMetadata(const std::string& filePath,
                               MeshMetadata& metadata,
                               MeshErrorCode& errorCode,
                               std::string& errorMsg);

    /**
     * @brief 获取格式的文件扩展名
     * @param format 网格格式
     * @return 文件扩展名（带点号，如".vtk"）
     */
    static std::string getFormatExtension(MeshFormat format);

    /**
     * @brief 获取格式的可读名称
     * @param format 网格格式
     * @return 格式的可读名称（如"VTK Legacy"）
     */
    static std::string getFormatName(MeshFormat format);

    /**
     * @brief 检查文件是否为指定格式
     * @param filePath 文件路径
     * @param format 目标格式
     * @return 是否为指定格式
     */
    static bool isFormat(const std::string& filePath, MeshFormat format);

private:
    /**
     * @brief 从文件路径获取扩展名
     * @param filePath 文件路径
     * @return 文件扩展名（小写，带点号）
     */
    static std::string getFileExtension(const std::string& filePath);
};
