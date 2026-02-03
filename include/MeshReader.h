#pragma once

#include <string>
#include "MeshTypes.h"
#include "MeshException.h"

/**
 * @brief 网格读取模块
 * 负责从文件读取网格数据到MeshData，支持自动格式识别和指定格式读取
 */
class MeshReader {
public:
    /**
     * @brief 自动识别文件格式并读取网格数据
     * @param filePath 文件路径（UTF-8编码）
     * @param[out] meshData 输出的网格数据
     * @param[out] errorCode 输出错误码
     * @param[out] errorMsg 输出错误信息（UTF-8）
     * @return 读取是否成功（true=成功，false=失败）
     */
    static bool readAuto(const std::string& filePath,
                         MeshData& meshData,
                         MeshErrorCode& errorCode,
                         std::string& errorMsg);

    /**
     * @brief 读取VTK格式文件（支持Legacy/XML自动识别）
     * @param filePath 文件路径（UTF-8编码）
     * @param[out] meshData 输出的网格数据
     * @param[out] errorCode 输出错误码
     * @param[out] errorMsg 输出错误信息（UTF-8）
     * @return 读取是否成功
     */
    static bool readVTK(const std::string& filePath,
                        MeshData& meshData,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg);

    /**
     * @brief 读取CGNS格式文件
     * @param filePath 文件路径（UTF-8编码）
     * @param[out] meshData 输出的网格数据
     * @param[out] errorCode 输出错误码
     * @param[out] errorMsg 输出错误信息（UTF-8）
     * @param baseIndex CGNS Base索引（默认0，第一个Base）
     * @param zoneIndex CGNS Zone索引（默认0，第一个Zone）
     * @return 读取是否成功
     */
    static bool readCGNS(const std::string& filePath,
                         MeshData& meshData,
                         MeshErrorCode& errorCode,
                         std::string& errorMsg,
                         int baseIndex = 0,
                         int zoneIndex = 0);

    /**
     * @brief 读取Gmsh格式文件（支持v2/v4自动识别）
     * @param filePath 文件路径（UTF-8编码）
     * @param[out] meshData 输出的网格数据
     * @param[out] errorCode 输出错误码
     * @param[out] errorMsg 输出错误信息（UTF-8）
     * @return 读取是否成功
     */
    static bool readGmsh(const std::string& filePath,
                         MeshData& meshData,
                         MeshErrorCode& errorCode,
                         std::string& errorMsg);

    /**
     * @brief 读取STL格式文件（ASCII/Binary）
     * @param filePath 文件路径（UTF-8编码）
     * @param[out] meshData 输出的网格数据
     * @param[out] errorCode 输出错误码
     * @param[out] errorMsg 输出错误信息（UTF-8）
     * @return 读取是否成功
     */
    static bool readSTL(const std::string& filePath,
                        MeshData& meshData,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg);

    /**
     * @brief 读取OBJ格式文件
     * @param filePath 文件路径（UTF-8编码）
     * @param[out] meshData 输出的网格数据
     * @param[out] errorCode 输出错误码
     * @param[out] errorMsg 输出错误信息（UTF-8）
     * @return 读取是否成功
     */
    static bool readOBJ(const std::string& filePath,
                        MeshData& meshData,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg);

    /**
     * @brief 读取PLY格式文件（ASCII/Binary）
     * @param filePath 文件路径（UTF-8编码）
     * @param[out] meshData 输出的网格数据
     * @param[out] errorCode 输出错误码
     * @param[out] errorMsg 输出错误信息（UTF-8）
     * @return 读取是否成功
     */
    static bool readPLY(const std::string& filePath,
                        MeshData& meshData,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg);

    /**
     * @brief 读取OFF格式文件
     * @param filePath 文件路径（UTF-8编码）
     * @param[out] meshData 输出的网格数据
     * @param[out] errorCode 输出错误码
     * @param[out] errorMsg 输出错误信息（UTF-8）
     * @return 读取是否成功
     */
    static bool readOFF(const std::string& filePath,
                        MeshData& meshData,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg);

    /**
     * @brief 读取SU2格式文件
     * @param filePath 文件路径（UTF-8编码）
     * @param[out] meshData 输出的网格数据
     * @param[out] errorCode 输出错误码
     * @param[out] errorMsg 输出错误信息（UTF-8）
     * @return 读取是否成功
     */
    static bool readSU2(const std::string& filePath,
                        MeshData& meshData,
                        MeshErrorCode& errorCode,
                        std::string& errorMsg);

    /**
     * @brief 读取OpenFOAM格式文件
     * @param filePath 文件路径（UTF-8编码）
     * @param[out] meshData 输出的网格数据
     * @param[out] errorCode 输出错误码
     * @param[out] errorMsg 输出错误信息（UTF-8）
     * @return 读取是否成功
     */
    static bool readOpenFOAM(const std::string& filePath,
                             MeshData& meshData,
                             MeshErrorCode& errorCode,
                             std::string& errorMsg);

private:
    /**
     * @brief 检查文件是否存在
     * @param filePath 文件路径
     * @return 是否存在
     */
    static bool fileExists(const std::string& filePath);

    /**
     * @brief 从文件头识别格式
     * @param filePath 文件路径
     * @return 识别出的格式
     */
    static MeshFormat detectFormatFromHeader(const std::string& filePath);
};
