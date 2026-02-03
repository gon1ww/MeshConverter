#pragma once

#include <string>
#include "MeshTypes.h"
#include "MeshException.h"

/**
 * @brief 网格写入模块
 * 负责将MeshData写入为指定格式的文件，支持格式特异性配置
 */
class MeshWriter {
public:
    /**
     * @brief 写入网格数据到指定格式文件
     * @param meshData 输入的网格数据
     * @param filePath 输出文件路径（UTF-8编码）
     * @param targetFormat 目标格式
     * @param options 写入选项（格式特异性配置）
     * @param[out] errorCode 输出错误码
     * @param[out] errorMsg 输出错误信息（UTF-8）
     * @return 写入是否成功
     */
    static bool write(const MeshData& meshData,
                      const std::string& filePath,
                      MeshFormat targetFormat,
                      const FormatWriteOptions& options,
                      MeshErrorCode& errorCode,
                      std::string& errorMsg);

    /**
     * @brief 写入VTK格式文件（自动区分Legacy/XML）
     * @param meshData 输入的网格数据
     * @param filePath 输出文件路径（UTF-8编码）
     * @param isXml 是否写入XML格式（false=Legacy，true=XML）
     * @param options 写入选项
     * @param[out] errorCode 输出错误码
     * @param[out] errorMsg 输出错误信息
     * @return 写入是否成功
     */
    static bool writeVTK(const MeshData& meshData,
                         const std::string& filePath,
                         bool isXml,
                         const FormatWriteOptions& options,
                         MeshErrorCode& errorCode,
                         std::string& errorMsg);

    /**
     * @brief 写入CGNS格式文件
     * @param meshData 输入的网格数据
     * @param filePath 输出文件路径（UTF-8编码）
     * @param options 写入选项（需指定cgnsBaseName/cgnsZoneName等）
     * @param[out] errorCode 输出错误码
     * @param[out] errorMsg 输出错误信息
     * @return 写入是否成功
     */
    static bool writeCGNS(const MeshData& meshData,
                          const std::string& filePath,
                          const FormatWriteOptions& options,
                          MeshErrorCode& errorCode,
                          std::string& errorMsg);

    /**
     * @brief 写入Gmsh格式文件
     * @param meshData 输入的网格数据
     * @param filePath 输出文件路径（UTF-8编码）
     * @param options 写入选项
     * @param[out] errorCode 输出错误码
     * @param[out] errorMsg 输出错误信息
     * @return 写入是否成功
     */
    static bool writeGmsh(const MeshData& meshData,
                          const std::string& filePath,
                          const FormatWriteOptions& options,
                          MeshErrorCode& errorCode,
                          std::string& errorMsg);

    /**
     * @brief 写入STL格式文件
     * @param meshData 输入的网格数据
     * @param filePath 输出文件路径（UTF-8编码）
     * @param options 写入选项
     * @param[out] errorCode 输出错误码
     * @param[out] errorMsg 输出错误信息
     * @return 写入是否成功
     */
    static bool writeSTL(const MeshData& meshData,
                         const std::string& filePath,
                         const FormatWriteOptions& options,
                         MeshErrorCode& errorCode,
                         std::string& errorMsg);

    /**
     * @brief 写入OBJ格式文件
     * @param meshData 输入的网格数据
     * @param filePath 输出文件路径（UTF-8编码）
     * @param options 写入选项
     * @param[out] errorCode 输出错误码
     * @param[out] errorMsg 输出错误信息
     * @return 写入是否成功
     */
    static bool writeOBJ(const MeshData& meshData,
                        const std::string& filePath,
                        const FormatWriteOptions& options,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg);

    /**
     * @brief 写入PLY格式文件
     * @param meshData 输入的网格数据
     * @param filePath 输出文件路径（UTF-8编码）
     * @param options 写入选项
     * @param[out] errorCode 输出错误码
     * @param[out] errorMsg 输出错误信息
     * @return 写入是否成功
     */
    static bool writePLY(const MeshData& meshData,
                        const std::string& filePath,
                        const FormatWriteOptions& options,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg);

    /**
     * @brief 写入OFF格式文件
     * @param meshData 输入的网格数据
     * @param filePath 输出文件路径（UTF-8编码）
     * @param options 写入选项
     * @param[out] errorCode 输出错误码
     * @param[out] errorMsg 输出错误信息
     * @return 写入是否成功
     */
    static bool writeOFF(const MeshData& meshData,
                        const std::string& filePath,
                        const FormatWriteOptions& options,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg);

    /**
     * @brief 写入SU2格式文件
     * @param meshData 输入的网格数据
     * @param filePath 输出文件路径（UTF-8编码）
     * @param options 写入选项
     * @param[out] errorCode 输出错误码
     * @param[out] errorMsg 输出错误信息
     * @return 写入是否成功
     */
    static bool writeSU2(const MeshData& meshData,
                        const std::string& filePath,
                        const FormatWriteOptions& options,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg);

    /**
     * @brief 写入OpenFOAM格式文件
     * @param meshData 输入的网格数据
     * @param filePath 输出文件路径（UTF-8编码）
     * @param options 写入选项
     * @param[out] errorCode 输出错误码
     * @param[out] errorMsg 输出错误信息
     * @return 写入是否成功
     */
    static bool writeOpenFOAM(const MeshData& meshData,
                             const std::string& filePath,
                             const FormatWriteOptions& options,
                             MeshErrorCode& errorCode,
                             std::string& errorMsg);

private:
    /**
     * @brief 确保目录存在
     * @param filePath 文件路径
     * @return 是否成功
     */
    static bool ensureDirectoryExists(const std::string& filePath);

    /**
     * @brief 检查网格数据是否为空
     * @param meshData 网格数据
     * @return 是否为空
     */
    static bool isMeshEmpty(const MeshData& meshData);
};
