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
 * @brief Format conversion module
 * Encapsulates the full "read-process-write" workflow, providing one-click format conversion capability
 */
class MeshConverter {
public:
    /**
     * @brief Single file format conversion
     * @param srcFilePath Source file path (UTF-8)
     * @param dstFilePath Target file path (UTF-8)
     * @param srcFormat Source format (MeshFormat::UNKNOWN=auto detect)
     * @param dstFormat Target format
     * @param writeOptions Target format write options
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message
     * @return Whether conversion is successful
     */
    static bool convert(const std::string& srcFilePath,
                       const std::string& dstFilePath,
                       MeshFormat srcFormat,
                       MeshFormat dstFormat,
                       const FormatWriteOptions& writeOptions,
                       MeshErrorCode& errorCode,
                       std::string& errorMsg);

    /**
     * @brief Batch convert multiple files
     * @param srcFilePaths Source file path list (UTF-8)
     * @param dstDir Target directory (UTF-8)
     * @param dstFormat Target format
     * @param writeOptions Target format write options
     * @param[out] errorMap Output error information for each file (key=source file path, value=(errorCode, errorMsg))
     * @return Number of successfully converted files
     */
    static uint64_t batchConvert(const std::vector<std::string>& srcFilePaths,
                                const std::string& dstDir,
                                MeshFormat dstFormat,
                                const FormatWriteOptions& writeOptions,
                                std::unordered_map<std::string, std::pair<MeshErrorCode, std::string>>& errorMap);

private:
    /**
     * @brief Generate target file path
     * @param srcFilePath Source file path
     * @param dstDir Target directory
     * @param dstFormat Target format
     * @return Target file path
     */
    static std::string generateDstFilePath(const std::string& srcFilePath,
                                          const std::string& dstDir,
                                          MeshFormat dstFormat);

    /**
     * @brief Ensure target directory exists
     * @param dstDir Target directory
     * @return Whether successful
     */
    static bool ensureDstDirExists(const std::string& dstDir);
};
