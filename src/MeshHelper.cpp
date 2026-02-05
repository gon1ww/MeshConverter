#include "MeshHelper.h"
#include <fstream>
#include <filesystem>
#include <algorithm>

/**
 * @brief Get file extension from file path
 * @param filePath File path
 * @return File extension (lowercase, with dot)
 */
std::string MeshHelper::getFileExtension(const std::string& filePath) {
    std::filesystem::path path(filePath);
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

/**
 * @brief Detect mesh format from file header
 * @param filePath File path (UTF-8)
 * @return Detected format (MeshFormat::UNKNOWN=unable to detect)
 */
MeshFormat MeshHelper::detectFormat(const std::string& filePath) {
    bool fileExists = std::filesystem::exists(filePath);

    // Try to determine from file extension first (works for both existing and non-existing files)
    std::string ext = getFileExtension(filePath);
    if (ext == ".vtk") {
        return MeshFormat::VTK_LEGACY;
    } else if (ext == ".vtu" || ext == ".vtp" || ext == ".vti" || ext == ".vts") {
        return MeshFormat::VTK_XML;
    } else if (ext == ".cgns") {
        return MeshFormat::CGNS;
    } else if (ext == ".msh") {
        // Need to further check Gmsh version if file exists
        if (fileExists) {
            std::ifstream file(filePath);
            if (file.is_open()) {
                std::string line;
                while (std::getline(file, line)) {
                    if (line.find("$MeshFormat") != std::string::npos) {
                        if (std::getline(file, line)) {
                            if (line.find("4.") != std::string::npos) {
                                return MeshFormat::GMSH_V4;
                            } else {
                                return MeshFormat::GMSH_V2;
                            }
                        }
                        break;
                    }
                }
                file.close();
            }
        }
        return MeshFormat::GMSH_V2; // Default to v2
    } else if (ext == ".stl") {
        // Check if STL is ASCII or Binary if file exists
        if (fileExists) {
            std::ifstream file(filePath, std::ios::binary);
            if (file.is_open()) {
                char header[80] = {0};
                file.read(header, sizeof(header));
                std::string headerStr(header);
                if (headerStr.find("solid") != std::string::npos || headerStr.find("SOLID") != std::string::npos) {
                    return MeshFormat::STL_ASCII;
                } else {
                    return MeshFormat::STL_BINARY;
                }
                file.close();
            }
        }
        return MeshFormat::STL_BINARY; // Default to binary
    } else if (ext == ".obj") {
        return MeshFormat::OBJ;
    } else if (ext == ".ply") {
        // Check if PLY is ASCII or Binary if file exists
        if (fileExists) {
            std::ifstream file(filePath);
            if (file.is_open()) {
                std::string line;
                if (std::getline(file, line) && line == "ply") {
                    while (std::getline(file, line)) {
                        if (line.find("format") != std::string::npos) {
                            if (line.find("ascii") != std::string::npos) {
                                return MeshFormat::PLY_ASCII;
                            } else {
                                return MeshFormat::PLY_BINARY;
                            }
                        }
                    }
                }
                file.close();
            }
        }
        return MeshFormat::PLY_ASCII; // Default to ascii
    } else if (ext == ".off") {
        return MeshFormat::OFF;
    } else if (ext == ".su2") {
        return MeshFormat::SU2;
    } else if (fileExists && std::filesystem::is_directory(filePath) && std::filesystem::exists(filePath + "/polyMesh")) {
        return MeshFormat::OPENFOAM;
    }

    // If cannot determine from extension, try from file header
    std::ifstream file(filePath, std::ios::binary);
    if (file.is_open()) {
        char header[128] = {0};
        file.read(header, sizeof(header));
        std::string headerStr(header);

        if (headerStr.find("# vtk") != std::string::npos) {
            return MeshFormat::VTK_LEGACY;
        } else if (headerStr.find("<?xml") != std::string::npos && headerStr.find("VTKFile") != std::string::npos) {
            return MeshFormat::VTK_XML;
        } else if (headerStr.find("CGNS") != std::string::npos) {
            return MeshFormat::CGNS;
        } else if (headerStr.find("$MeshFormat") != std::string::npos) {
            return MeshFormat::GMSH_V2; // Default to v2
        } else if (headerStr.find("solid") != std::string::npos || headerStr.find("SOLID") != std::string::npos) {
            return MeshFormat::STL_ASCII;
        } else if (headerStr.find("ply") != std::string::npos) {
            return MeshFormat::PLY_ASCII;
        } else if (headerStr.find("OFF") != std::string::npos) {
            return MeshFormat::OFF;
        } else if (headerStr.find("SU2_MESH") != std::string::npos) {
            return MeshFormat::SU2;
        }

        file.close();
    }

    return MeshFormat::UNKNOWN;
}

/**
 * @brief Extract mesh metadata (without loading full geometry/topology data, improve performance)
 * @param filePath File path (UTF-8)
 * @param[out] metadata Output metadata
 * @param[out] errorCode Output error code
 * @param[out] errorMsg Output error message
 * @return Whether extraction is successful
 */
bool MeshHelper::extractMetadata(const std::string& filePath,
                               MeshMetadata& metadata,
                               MeshErrorCode& errorCode,
                               std::string& errorMsg) {
    // Check if file exists
    if (!std::filesystem::exists(filePath)) {
        errorCode = MeshErrorCode::FILE_NOT_EXIST;
        errorMsg = "File not exist: " + filePath;
        return false;
    }

    // Detect format
    MeshFormat format = detectFormat(filePath);
    if (format == MeshFormat::UNKNOWN) {
        errorCode = MeshErrorCode::FORMAT_UNSUPPORTED;
        errorMsg = "Unable to detect file format: " + filePath;
        return false;
    }

    // Fill basic metadata
    metadata.fileName = std::filesystem::path(filePath).filename().string();
    metadata.format = format;

    // Extract detailed metadata based on format
    // Basic metadata extraction logic here
    // For complex formats, need to use corresponding libraries

    // Temporarily return success, but only fill basic information
    metadata.meshType = MeshType::UNKNOWN;
    metadata.pointCount = 0;
    metadata.cellCount = 0;
    metadata.formatVersion = "unknown";

    return true;
}

/**
 * @brief Get file extension for format
 * @param format Mesh format
 * @return File extension (with dot, e.g., ".vtk")
 */
std::string MeshHelper::getFormatExtension(MeshFormat format) {
    switch (format) {
        case MeshFormat::VTK_LEGACY:
            return ".vtk";
        case MeshFormat::VTK_XML:
            return ".vtu";
        case MeshFormat::CGNS:
            return ".cgns";
        case MeshFormat::GMSH_V2:
        case MeshFormat::GMSH_V4:
            return ".msh";
        case MeshFormat::SU2:
            return ".su2";
        case MeshFormat::OPENFOAM:
            return "";
        case MeshFormat::STL_ASCII:
        case MeshFormat::STL_BINARY:
            return ".stl";
        case MeshFormat::OBJ:
            return ".obj";
        case MeshFormat::PLY_ASCII:
        case MeshFormat::PLY_BINARY:
            return ".ply";
        case MeshFormat::OFF:
            return ".off";
        default:
            return "";
    }
}

/**
 * @brief Get readable name for format
 * @param format Mesh format
 * @return Readable name for format (e.g., "VTK Legacy")
 */
std::string MeshHelper::getFormatName(MeshFormat format) {
    switch (format) {
        case MeshFormat::VTK_LEGACY:
            return "VTK Legacy";
        case MeshFormat::VTK_XML:
            return "VTK XML";
        case MeshFormat::CGNS:
            return "CGNS";
        case MeshFormat::GMSH_V2:
            return "Gmsh v2";
        case MeshFormat::GMSH_V4:
            return "Gmsh v4";
        case MeshFormat::SU2:
            return "SU2";
        case MeshFormat::OPENFOAM:
            return "OpenFOAM";
        case MeshFormat::STL_ASCII:
            return "STL ASCII";
        case MeshFormat::STL_BINARY:
            return "STL Binary";
        case MeshFormat::OBJ:
            return "OBJ";
        case MeshFormat::PLY_ASCII:
            return "PLY ASCII";
        case MeshFormat::PLY_BINARY:
            return "PLY Binary";
        case MeshFormat::OFF:
            return "OFF";
        default:
            return "Unknown";
    }
}

/**
 * @brief Check if file is of specified format
 * @param filePath File path
 * @param format Target format
 * @return Whether file is of specified format
 */
bool MeshHelper::isFormat(const std::string& filePath, MeshFormat format) {
    return detectFormat(filePath) == format;
}

/**
 * @brief Detect format from file extension only (works for non-existent files)
 * @param filePath File path (UTF-8)
 * @return Detected format (MeshFormat::UNKNOWN=unable to detect)
 */
MeshFormat MeshHelper::detectFormatFromExtension(const std::string& filePath) {
    try {
        std::string ext = getFileExtension(filePath);
        if (ext == ".vtk") {
            return MeshFormat::VTK_LEGACY;
        } else if (ext == ".vtu" || ext == ".vtp" || ext == ".vti" || ext == ".vts") {
            return MeshFormat::VTK_XML;
        } else if (ext == ".cgns") {
            return MeshFormat::CGNS;
        } else if (ext == ".msh") {
            return MeshFormat::GMSH_V4;
        } else if (ext == ".stl") {
            return MeshFormat::STL_BINARY;
        } else if (ext == ".obj") {
            return MeshFormat::OBJ;
        } else if (ext == ".ply") {
            return MeshFormat::PLY_ASCII;
        } else if (ext == ".off") {
            return MeshFormat::OFF;
        } else if (ext == ".su2") {
            return MeshFormat::SU2;
        }
        
        return MeshFormat::UNKNOWN;
    } catch (const std::exception& e) {
        return MeshFormat::UNKNOWN;
    }
}

/**
 * @brief Check if a format is supported
 * @param format Mesh format to check
 * @return Whether the format is supported
 */
bool MeshHelper::isSupportedFormat(MeshFormat format) {
    switch (format) {
        case MeshFormat::VTK_LEGACY:
        case MeshFormat::VTK_XML:
        case MeshFormat::CGNS:
        case MeshFormat::GMSH_V2:
        case MeshFormat::GMSH_V4:
        case MeshFormat::SU2:
        case MeshFormat::OPENFOAM:
        case MeshFormat::STL_ASCII:
        case MeshFormat::STL_BINARY:
        case MeshFormat::OBJ:
        case MeshFormat::PLY_ASCII:
        case MeshFormat::PLY_BINARY:
        case MeshFormat::OFF:
            return true;
        default:
            return false;
    }
}

/**
 * @brief Get list of all supported formats
 * @return Vector of supported formats
 */
std::vector<MeshFormat> MeshHelper::getSupportedFormats() {
    return {
        MeshFormat::VTK_LEGACY,
        MeshFormat::VTK_XML,
        MeshFormat::CGNS,
        MeshFormat::GMSH_V2,
        MeshFormat::GMSH_V4,
        MeshFormat::SU2,
        MeshFormat::OPENFOAM,
        MeshFormat::STL_ASCII,
        MeshFormat::STL_BINARY,
        MeshFormat::OBJ,
        MeshFormat::PLY_ASCII,
        MeshFormat::PLY_BINARY,
        MeshFormat::OFF
    };
}

/**
 * @brief Get list of supported format names for display
 * @return Vector of readable format names
 */
std::vector<std::string> MeshHelper::getSupportedFormatNames() {
    return {
        "VTK Legacy (.vtk)",
        "VTK XML (.vtu/.vtp/.vti/.vts)",
        "CGNS (.cgns)",
        "Gmsh v2 (.msh)",
        "Gmsh v4 (.msh)",
        "SU2 (.su2)",
        "OpenFOAM",
        "STL ASCII (.stl)",
        "STL Binary (.stl)",
        "OBJ (.obj)",
        "PLY ASCII (.ply)",
        "PLY Binary (.ply)",
        "OFF (.off)"
    };
}

/**
 * @brief Validate output file format
 * @param filePath Output file path
 * @param[out] errorMsg Error message if validation fails
 * @return Whether the format is valid
 */
bool MeshHelper::validateOutputFormat(const std::string& filePath, std::string& errorMsg) {
    try {
        MeshFormat format = detectFormatFromExtension(filePath);
        
        if (format == MeshFormat::UNKNOWN) {
            errorMsg = getUnsupportedFormatMessage(filePath);
            return false;
        }
        
        if (!isSupportedFormat(format)) {
            errorMsg = getUnsupportedFormatMessage(filePath);
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        errorMsg = std::string("Error validating output format: ") + e.what();
        return false;
    }
}

/**
 * @brief Get comprehensive error message for unsupported format
 * @param filePath File path that caused the error
 * @return Detailed error message with guidance
 */
std::string MeshHelper::getUnsupportedFormatMessage(const std::string& filePath) {
    std::string ext = getFileExtension(filePath);
    std::string message = "Error: Cannot detect output file format for \"" + filePath + "\"\n";
    
    if (ext.empty()) {
        message += "  - No file extension found in the output path\n";
    } else {
        message += "  - Unsupported file extension: \"" + ext + "\"\n";
    }
    
    message += "\nSupported output formats and extensions:\n";
    std::vector<std::string> formatNames = getSupportedFormatNames();
    for (const auto& name : formatNames) {
        message += "  - " + name + "\n";
    }
    
    message += "\nSolutions:\n";
    message += "  1. Use one of the supported file extensions listed above\n";
    message += "  2. Or specify the target format explicitly using --target-format option\n";
    message += "     Example: --target-format vtk\n";
    
    return message;
}
