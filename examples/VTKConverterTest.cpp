#include <iostream>
#include <string>
#include "VTKConverter.h"
#include "MeshTypes.h"

/**
 * @brief VTK intermediate format conversion test program
 * Supports specifying input file path and output format, using VTK as core intermediate format
 */
int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc < 3) {
        std::cerr << "Usage: VTKConverterTest <input file path> <output format> [output file path]" << std::endl;
        std::cerr << "Supported output formats: vtk, cgns, gmsh, stl, obj, ply, off, su2, openfoam" << std::endl;
        return 1;
    }

    // Parse command line arguments
    std::string inputPath = argv[1];
    std::string outputFormatStr = argv[2];
    std::string outputPath;

    if (argc >= 4) {
        outputPath = argv[3];
    } else {
        // Auto generate output file path
        size_t lastDot = inputPath.find_last_of('.');
        if (lastDot != std::string::npos) {
            outputPath = inputPath.substr(0, lastDot) + "." + outputFormatStr;
        } else {
            outputPath = inputPath + "." + outputFormatStr;
        }
    }

    // Convert output format string to MeshFormat enum
    MeshFormat targetFormat = MeshFormat::UNKNOWN;
    if (outputFormatStr == "vtk") {
        targetFormat = MeshFormat::VTK_LEGACY;
    } else if (outputFormatStr == "cgns") {
        targetFormat = MeshFormat::CGNS;
    } else if (outputFormatStr == "gmsh") {
        targetFormat = MeshFormat::GMSH_V4;
    } else if (outputFormatStr == "stl") {
        targetFormat = MeshFormat::STL_BINARY;
    } else if (outputFormatStr == "obj") {
        targetFormat = MeshFormat::OBJ;
    } else if (outputFormatStr == "ply") {
        targetFormat = MeshFormat::PLY_BINARY;
    } else if (outputFormatStr == "off") {
        targetFormat = MeshFormat::OFF;
    } else if (outputFormatStr == "su2") {
        targetFormat = MeshFormat::SU2;
    } else if (outputFormatStr == "openfoam") {
        targetFormat = MeshFormat::OPENFOAM;
    } else {
        std::cerr << "Unsupported output format: " << outputFormatStr << std::endl;
        return 1;
    }

    std::cout << "Input file: " << inputPath << std::endl;
    std::cout << "Output format: " << outputFormatStr << std::endl;
    std::cout << "Output file: " << outputPath << std::endl;

    // Use VTKConverter to test complete conversion workflow
    std::cout << "\nTesting VTKConverter complete workflow..." << std::endl;
    MeshErrorCode errorCode;
    std::string errorMsg;
    
    // Set up processing options
    VTKConverter::VTKProcessingOptions processingOptions;
    processingOptions.enableCleaning = true;
    processingOptions.enableTriangulation = false;
    processingOptions.enableDecimation = false;
    processingOptions.enableSmoothing = false;
    processingOptions.enableNormalComputation = false;
    
    // Set up write options
    FormatWriteOptions writeOptions;
    
    // Detect source format
    MeshFormat sourceFormat = MeshFormat::UNKNOWN;
    if (inputPath.find(".stl") != std::string::npos) {
        sourceFormat = MeshFormat::STL_BINARY;
    } else if (inputPath.find(".obj") != std::string::npos) {
        sourceFormat = MeshFormat::OBJ;
    } else if (inputPath.find(".ply") != std::string::npos) {
        sourceFormat = MeshFormat::PLY_BINARY;
    } else if (inputPath.find(".vtk") != std::string::npos) {
        sourceFormat = MeshFormat::VTK_LEGACY;
    } else if (inputPath.find(".cgns") != std::string::npos) {
        sourceFormat = MeshFormat::CGNS;
    } else if (inputPath.find(".msh") != std::string::npos) {
        sourceFormat = MeshFormat::GMSH_V4;
    } else if (inputPath.find(".off") != std::string::npos) {
        sourceFormat = MeshFormat::OFF;
    } else if (inputPath.find(".su2") != std::string::npos) {
        sourceFormat = MeshFormat::SU2;
    } else {
        sourceFormat = MeshFormat::UNKNOWN;
    }
    
    // Run conversion
    bool success = VTKConverter::convert(inputPath, outputPath, sourceFormat, targetFormat, processingOptions, writeOptions, errorCode, errorMsg);
    
    if (!success) {
        std::cerr << "Conversion failed: " << errorMsg << " (Error code: " << static_cast<int>(errorCode) << ")" << std::endl;
        return 1;
    }
    
    std::cout << "\nConversion completed successfully!" << std::endl;
    std::cout << "Source: " << inputPath << std::endl;
    std::cout << "Destination: " << outputPath << std::endl;

    return 0;
}
