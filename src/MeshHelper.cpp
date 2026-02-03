#include "MeshHelper.h"
#include <fstream>
#include <filesystem>
#include <algorithm>

/**
 * @brief 从文件路径获取扩展名
 * @param filePath 文件路径
 * @return 文件扩展名（小写，带点号）
 */
std::string MeshHelper::getFileExtension(const std::string& filePath) {
    std::filesystem::path path(filePath);
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext;
}

/**
 * @brief 从文件头识别网格格式
 * @param filePath 文件路径（UTF-8）
 * @return 识别出的格式（MeshFormat::UNKNOWN=无法识别）
 */
MeshFormat MeshHelper::detectFormat(const std::string& filePath) {
    // 首先检查文件是否存在
    if (!std::filesystem::exists(filePath)) {
        return MeshFormat::UNKNOWN;
    }

    // 尝试从文件扩展名判断
    std::string ext = getFileExtension(filePath);
    if (ext == ".vtk") {
        return MeshFormat::VTK_LEGACY;
    } else if (ext == ".vtu" || ext == ".vtp" || ext == ".vti" || ext == ".vts") {
        return MeshFormat::VTK_XML;
    } else if (ext == ".cgns") {
        return MeshFormat::CGNS;
    } else if (ext == ".msh") {
        // 需要进一步检查Gmsh版本
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
        return MeshFormat::GMSH_V2; // 默认返回v2
    } else if (ext == ".stl") {
        // 检查STL是ASCII还是Binary
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
        return MeshFormat::STL_BINARY; // 默认返回binary
    } else if (ext == ".obj") {
        return MeshFormat::OBJ;
    } else if (ext == ".ply") {
        // 检查PLY是ASCII还是Binary
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
        return MeshFormat::PLY_ASCII; // 默认返回ascii
    } else if (ext == ".off") {
        return MeshFormat::OFF;
    } else if (ext == ".su2") {
        return MeshFormat::SU2;
    } else if (std::filesystem::is_directory(filePath) && std::filesystem::exists(filePath + "/polyMesh")) {
        return MeshFormat::OPENFOAM;
    }

    // 如果无法从扩展名判断，尝试从文件头判断
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
            return MeshFormat::GMSH_V2; // 默认返回v2
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
 * @brief 提取网格元数据（不加载完整几何/拓扑数据，提升性能）
 * @param filePath 文件路径（UTF-8）
 * @param[out] metadata 输出元数据
 * @param[out] errorCode 输出错误码
 * @param[out] errorMsg 输出错误信息
 * @return 提取是否成功
 */
bool MeshHelper::extractMetadata(const std::string& filePath,
                               MeshMetadata& metadata,
                               MeshErrorCode& errorCode,
                               std::string& errorMsg) {
    // 检查文件是否存在
    if (!std::filesystem::exists(filePath)) {
        errorCode = MeshErrorCode::FILE_NOT_EXIST;
        errorMsg = "文件不存在: " + filePath;
        return false;
    }

    // 识别格式
    MeshFormat format = detectFormat(filePath);
    if (format == MeshFormat::UNKNOWN) {
        errorCode = MeshErrorCode::FORMAT_UNSUPPORTED;
        errorMsg = "无法识别文件格式: " + filePath;
        return false;
    }

    // 填充基本元数据
    metadata.fileName = std::filesystem::path(filePath).filename().string();
    metadata.format = format;

    // 根据格式提取详细元数据
    // 这里实现基本的元数据提取逻辑
    // 对于复杂格式，需要使用相应的库

    // 暂时返回成功，但只填充基本信息
    metadata.meshType = MeshType::UNKNOWN;
    metadata.pointCount = 0;
    metadata.cellCount = 0;
    metadata.formatVersion = "unknown";

    return true;
}

/**
 * @brief 获取格式的文件扩展名
 * @param format 网格格式
 * @return 文件扩展名（带点号，如".vtk"）
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
 * @brief 获取格式的可读名称
 * @param format 网格格式
 * @return 格式的可读名称（如"VTK Legacy"）
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
 * @brief 检查文件是否为指定格式
 * @param filePath 文件路径
 * @param format 目标格式
 * @return 是否为指定格式
 */
bool MeshHelper::isFormat(const std::string& filePath, MeshFormat format) {
    return detectFormat(filePath) == format;
}
