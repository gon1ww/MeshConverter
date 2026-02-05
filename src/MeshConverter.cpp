#include "MeshConverter.h"
#include <filesystem>
#include <thread>
#include <mutex>

/**
 * @brief Generate target file path
 * @param srcFilePath Source file path
 * @param dstDir Target directory
 * @param dstFormat Target format
 * @return Target file path
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
 * @brief Ensure target directory exists
 * @param dstDir Target directory
 * @return Whether successful
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
 * @brief Single file format conversion
 * @param srcFilePath Source file path (UTF-8)
 * @param dstFilePath Target file path (UTF-8)
 * @param srcFormat Source format (MeshFormat::UNKNOWN=auto-detect)
 * @param dstFormat Target format
 * @param writeOptions Target format write options
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message
 * @return Whether conversion is successful
 */
bool MeshConverter::convert(const std::string& srcFilePath,
                           const std::string& dstFilePath,
                           MeshFormat srcFormat,
                           MeshFormat dstFormat,
                           const FormatWriteOptions& writeOptions,
                           MeshErrorCode& errorCode,
                           std::string& errorMsg) {
    // Check if source file exists
    if (!std::filesystem::exists(srcFilePath)) {
        errorCode = MeshErrorCode::FILE_NOT_EXIST;
        errorMsg = "Source file does not exist: " + srcFilePath;
        return false;
    }

    // Ensure target directory exists
    std::filesystem::path dstPath(dstFilePath);
    std::filesystem::path dstDir = dstPath.parent_path();
    if (!dstDir.empty() && !std::filesystem::exists(dstDir)) {
        try {
            std::filesystem::create_directories(dstDir);
        } catch (...) {
            errorCode = MeshErrorCode::WRITE_FAILED;
            errorMsg = "Cannot create target directory: " + dstDir.string();
            return false;
        }
    }

    // Read mesh data
    MeshData meshData;
    MeshErrorCode readErrorCode;
    std::string readErrorMsg;

    bool readSuccess = false;
    if (srcFormat == MeshFormat::UNKNOWN) {
        // Auto-detect format and read
        readSuccess = MeshReader::readAuto(srcFilePath, meshData, readErrorCode, readErrorMsg);
    } else {
        // Read according to specified format
        // Need to call corresponding read method based on specific format
        // Temporarily use readAuto as default implementation
        readSuccess = MeshReader::readAuto(srcFilePath, meshData, readErrorCode, readErrorMsg);
    }

    if (!readSuccess) {
        errorCode = readErrorCode;
        errorMsg = "Read failed: " + readErrorMsg;
        return false;
    }

    // Write mesh data
    MeshErrorCode writeErrorCode;
    std::string writeErrorMsg;
    bool writeSuccess = MeshWriter::write(meshData, dstFilePath, dstFormat, writeOptions, writeErrorCode, writeErrorMsg);

    if (!writeSuccess) {
        errorCode = writeErrorCode;
        errorMsg = "Write failed: " + writeErrorMsg;
        return false;
    }

    return true;
}

/**
 * @brief Batch convert multiple files
 * @param srcFilePaths Source file path list (UTF-8)
 * @param dstDir Target directory (UTF-8)
 * @param dstFormat Target format
 * @param writeOptions Target format write options
 * @param[out] errorMap Output error information for each file (key=source file path, value=(errorCode, errorMsg))
 * @return Number of successfully converted files
 */
uint64_t MeshConverter::batchConvert(const std::vector<std::string>& srcFilePaths,
                                    const std::string& dstDir,
                                    MeshFormat dstFormat,
                                    const FormatWriteOptions& writeOptions,
                                    std::unordered_map<std::string, std::pair<MeshErrorCode, std::string>>& errorMap) {
    // Ensure target directory exists
    if (!ensureDstDirExists(dstDir)) {
        errorMap.clear();
        for (const auto& filePath : srcFilePaths) {
            errorMap[filePath] = {MeshErrorCode::WRITE_FAILED, "Cannot create target directory: " + dstDir};
        }
        return 0;
    }

    // Number of successfully converted files
    uint64_t successCount = 0;
    std::mutex errorMapMutex;

    // Use multi-threading for batch conversion
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

    // Wait for remaining threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    return successCount;
}
