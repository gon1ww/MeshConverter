#include "MeshConverter.h"
#include <filesystem>
#include <thread>
#include <mutex>

/**
 * @brief 生成目标文件路径
 * @param srcFilePath 源文件路径
 * @param dstDir 目标目录
 * @param dstFormat 目标格式
 * @return 目标文件路径
 */
std::string MeshConverter::generateDstFilePath(const std::string& srcFilePath,
                                              const std::string& dstDir,
                                              MeshFormat dstFormat) {
    std::filesystem::path srcPath(srcFilePath);
    std::string fileName = srcPath.stem().string();
    std::string extension = MeshHelper::getFormatExtension(dstFormat);
    std::filesystem::path dstPath(dstDir);
    dstPath /= (fileName + extension);
    return dstPath.string();
}

/**
 * @brief 确保目标目录存在
 * @param dstDir 目标目录
 * @return 是否成功
 */
bool MeshConverter::ensureDstDirExists(const std::string& dstDir) {
    if (!std::filesystem::exists(dstDir)) {
        try {
            std::filesystem::create_directories(dstDir);
            return true;
        } catch (...) {
            return false;
        }
    }
    return true;
}

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
bool MeshConverter::convert(const std::string& srcFilePath,
                           const std::string& dstFilePath,
                           MeshFormat srcFormat,
                           MeshFormat dstFormat,
                           const FormatWriteOptions& writeOptions,
                           MeshErrorCode& errorCode,
                           std::string& errorMsg) {
    // 检查源文件是否存在
    if (!std::filesystem::exists(srcFilePath)) {
        errorCode = MeshErrorCode::FILE_NOT_EXIST;
        errorMsg = "源文件不存在: " + srcFilePath;
        return false;
    }

    // 确保目标目录存在
    std::filesystem::path dstPath(dstFilePath);
    std::filesystem::path dstDir = dstPath.parent_path();
    if (!dstDir.empty() && !std::filesystem::exists(dstDir)) {
        try {
            std::filesystem::create_directories(dstDir);
        } catch (...) {
            errorCode = MeshErrorCode::WRITE_FAILED;
            errorMsg = "无法创建目标目录: " + dstDir.string();
            return false;
        }
    }

    // 读取网格数据
    MeshData meshData;
    MeshErrorCode readErrorCode;
    std::string readErrorMsg;

    bool readSuccess = false;
    if (srcFormat == MeshFormat::UNKNOWN) {
        // 自动识别格式并读取
        readSuccess = MeshReader::readAuto(srcFilePath, meshData, readErrorCode, readErrorMsg);
    } else {
        // 根据指定格式读取
        // 这里需要根据具体格式调用对应的读取方法
        // 暂时使用readAuto作为默认实现
        readSuccess = MeshReader::readAuto(srcFilePath, meshData, readErrorCode, readErrorMsg);
    }

    if (!readSuccess) {
        errorCode = readErrorCode;
        errorMsg = "读取失败: " + readErrorMsg;
        return false;
    }

    // 写入网格数据
    MeshErrorCode writeErrorCode;
    std::string writeErrorMsg;
    bool writeSuccess = MeshWriter::write(meshData, dstFilePath, dstFormat, writeOptions, writeErrorCode, writeErrorMsg);

    if (!writeSuccess) {
        errorCode = writeErrorCode;
        errorMsg = "写入失败: " + writeErrorMsg;
        return false;
    }

    return true;
}

/**
 * @brief 批量转换多个文件
 * @param srcFilePaths 源文件路径列表（UTF-8）
 * @param dstDir 目标目录（UTF-8）
 * @param dstFormat 目标格式
 * @param writeOptions 目标格式写入选项
 * @param[out] errorMap 输出每个文件的错误信息（key=源文件路径，value=(errorCode, errorMsg)）
 * @return 成功转换的文件数量
 */
uint64_t MeshConverter::batchConvert(const std::vector<std::string>& srcFilePaths,
                                    const std::string& dstDir,
                                    MeshFormat dstFormat,
                                    const FormatWriteOptions& writeOptions,
                                    std::unordered_map<std::string, std::pair<MeshErrorCode, std::string>>& errorMap) {
    // 确保目标目录存在
    if (!ensureDstDirExists(dstDir)) {
        errorMap.clear();
        for (const auto& filePath : srcFilePaths) {
            errorMap[filePath] = {MeshErrorCode::WRITE_FAILED, "无法创建目标目录: " + dstDir};
        }
        return 0;
    }

    // 成功转换的文件数量
    uint64_t successCount = 0;
    std::mutex errorMapMutex;

    // 使用多线程批量转换
    std::vector<std::thread> threads;
    const size_t maxThreads = std::thread::hardware_concurrency();
    size_t currentThread = 0;

    for (const auto& srcFilePath : srcFilePaths) {
        threads.emplace_back([&, srcFilePath]() {
            std::string dstFilePath = generateDstFilePath(srcFilePath, dstDir, dstFormat);
            MeshErrorCode errorCode;
            std::string errorMsg;

            bool success = convert(srcFilePath, dstFilePath, MeshFormat::UNKNOWN, dstFormat, writeOptions, errorCode, errorMsg);

            if (success) {
                std::lock_guard<std::mutex> lock(errorMapMutex);
                successCount++;
            } else {
                std::lock_guard<std::mutex> lock(errorMapMutex);
                errorMap[srcFilePath] = {errorCode, errorMsg};
            }
        });

        currentThread++;
        if (currentThread >= maxThreads) {
            for (auto& thread : threads) {
                thread.join();
            }
            threads.clear();
            currentThread = 0;
        }
    }

    // 等待剩余线程完成
    for (auto& thread : threads) {
        thread.join();
    }

    return successCount;
}
