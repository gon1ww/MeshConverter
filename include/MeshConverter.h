#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include "MeshTypes.h"
#include "MeshException.h"
#include "MeshReader.h"
#include "MeshWriter.h"
#include "MeshHelper.h"

/**
 * @brief 格式转换模块
 * 封装"读取-处理-写入"全流程，提供一键格式转换能力
 */
class MeshConverter {
public:
    /**
     * @brief 单文件格式转换
     * @param srcFilePath 源文件路径（UTF-8）
     * @param dstFilePath 目标文件路径（UTF-8）
     * @param srcFormat 源格式（MeshFormat::UNKNOWN=自动识别）
     * @param dstFormat 目标格式
     * @param writeOptions 目标格式写入选项
     * @param[out] errorCode 输出错误码
     * @param[out] errorMsg 输出错误信息
     * @return 转换是否成功
     */
    static bool convert(const std::string& srcFilePath,
                       const std::string& dstFilePath,
                       MeshFormat srcFormat,
                       MeshFormat dstFormat,
                       const FormatWriteOptions& writeOptions,
                       MeshErrorCode& errorCode,
                       std::string& errorMsg);

    /**
     * @brief 批量转换多个文件
     * @param srcFilePaths 源文件路径列表（UTF-8）
     * @param dstDir 目标目录（UTF-8）
     * @param dstFormat 目标格式
     * @param writeOptions 目标格式写入选项
     * @param[out] errorMap 输出每个文件的错误信息（key=源文件路径，value=(errorCode, errorMsg)）
     * @return 成功转换的文件数量
     */
    static uint64_t batchConvert(const std::vector<std::string>& srcFilePaths,
                                const std::string& dstDir,
                                MeshFormat dstFormat,
                                const FormatWriteOptions& writeOptions,
                                std::unordered_map<std::string, std::pair<MeshErrorCode, std::string>>& errorMap);

private:
    /**
     * @brief 生成目标文件路径
     * @param srcFilePath 源文件路径
     * @param dstDir 目标目录
     * @param dstFormat 目标格式
     * @return 目标文件路径
     */
    static std::string generateDstFilePath(const std::string& srcFilePath,
                                          const std::string& dstDir,
                                          MeshFormat dstFormat);

    /**
     * @brief 确保目标目录存在
     * @param dstDir 目标目录
     * @return 是否成功
     */
    static bool ensureDstDirExists(const std::string& dstDir);
};
