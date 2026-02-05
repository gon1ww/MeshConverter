#pragma once

#include <string>
#include <vector>
#include "MeshTypes.h"
#include "MeshException.h"

/**
 * @brief Helper interface module
 * Provides format detection, metadata extraction and other auxiliary capabilities
 */
class MeshHelper {
public:
    /**
     * @brief Detect mesh format from file header or extension
     * @param filePath File path (UTF-8)
     * @return Detected format (MeshFormat::UNKNOWN=unable to detect)
     */
    static MeshFormat detectFormat(const std::string& filePath);

    /**
     * @brief Detect format from file extension only (works for non-existent files)
     * @param filePath File path (UTF-8)
     * @return Detected format (MeshFormat::UNKNOWN=unable to detect)
     */
    static MeshFormat detectFormatFromExtension(const std::string& filePath);

    /**
     * @brief Check if a format is supported
     * @param format Mesh format to check
     * @return Whether the format is supported
     */
    static bool isSupportedFormat(MeshFormat format);

    /**
     * @brief Get list of all supported formats
     * @return Vector of supported formats
     */
    static std::vector<MeshFormat> getSupportedFormats();

    /**
     * @brief Get list of supported format names for display
     * @return Vector of readable format names
     */
    static std::vector<std::string> getSupportedFormatNames();

    /**
     * @brief Validate output file format
     * @param filePath Output file path
     * @param[out] errorMsg Error message if validation fails
     * @return Whether the format is valid
     */
    static bool validateOutputFormat(const std::string& filePath, std::string& errorMsg);

    /**
     * @brief Extract mesh metadata (without loading complete geometry/topology data, improves performance)
     * @param filePath File path (UTF-8)
     * @param[out] metadata Output metadata
     * @param[out] errorCode Output error code
     * @param[out] errorMsg Output error message
     * @return Whether extraction is successful
     */
    static bool extractMetadata(const std::string& filePath,
                               MeshMetadata& metadata,
                               MeshErrorCode& errorCode,
                               std::string& errorMsg);

    /**
     * @brief Get file extension for format
     * @param format Mesh format
     * @return File extension (with dot, e.g., ".vtk")
     */
    static std::string getFormatExtension(MeshFormat format);

    /**
     * @brief Get readable name for format
     * @param format Mesh format
     * @return Readable format name (e.g., "VTK Legacy")
     */
    static std::string getFormatName(MeshFormat format);

    /**
     * @brief Check if file is of specified format
     * @param filePath File path
     * @param format Target format
     * @return Whether it is the specified format
     */
    static bool isFormat(const std::string& filePath, MeshFormat format);

    /**
     * @brief Get comprehensive error message for unsupported format
     * @param filePath File path that caused the error
     * @return Detailed error message with guidance
     */
    static std::string getUnsupportedFormatMessage(const std::string& filePath);

private:
    /**
     * @brief Get file extension from file path
     * @param filePath File path
     * @return File extension (lowercase, with dot)
     */
    static std::string getFileExtension(const std::string& filePath);
};
