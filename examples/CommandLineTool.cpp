#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "MeshConverter.h"
#include "MeshHelper.h"
#include "VTKConverter.h"

struct CommandLineOptions {
    std::string inputFile;
    std::string outputFile;
    std::string sourceFormat;
    std::string targetFormat;
    bool help = false;
    bool version = false;
    bool listFormats = false;
    bool verbose = false;
    VTKConverter::VTKProcessingOptions processingOptions;
};

void printHelp() {
    std::cout << "Mesh Format Converter Command Line Tool v1.0" << std::endl;
    std::cout << "Usage: meshconv [options] <input file> <output file>" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help             Show this help message" << std::endl;
    std::cout << "  -v, --version          Show version information" << std::endl;
    std::cout << "  -l, --list-formats     List supported formats" << std::endl;
    std::cout << "  -s, --source-format    Specify source file format" << std::endl;
    std::cout << "  -t, --target-format    Specify target file format" << std::endl;
    std::cout << "  -V, --verbose          Enable verbose output" << std::endl;
    std::cout << "  --no-cleaning          Disable point cleaning" << std::endl;
    std::cout << "  --triangulate          Enable triangulation" << std::endl;
    std::cout << "  --decimate <factor>    Enable mesh decimation, specify factor(0.0-1.0)" << std::endl;
    std::cout << "  --smooth <iterations>  Enable mesh smoothing, specify iterations" << std::endl;
    std::cout << "  --compute-normals      Compute normal vectors" << std::endl;
    std::cout << std::endl;
    std::cout << "Supported formats:" << std::endl;
    std::cout << "  Volume meshes: vtk, vtu, cgns, msh, su2" << std::endl;
    std::cout << "  Surface meshes: stl, obj, ply, off" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  meshconv input.stl output.vtk" << std::endl;
    std::cout << "  meshconv --source-format stl --target-format vtk input.stl output.vtk" << std::endl;
    std::cout << "  meshconv --decimate 0.5 --smooth 10 input.stl output.obj" << std::endl;
}

void printVersion() {
    std::cout << "Mesh Format Converter Command Line Tool v1.0" << std::endl;
    std::cout << "Based on VTK 9.x, CGNS 4.x, Gmsh 4.x" << std::endl;
    std::cout << "Supports multiple mesh formats reading and conversion" << std::endl;
}

void printSupportedFormats() {
    std::cout << "Supported mesh formats:" << std::endl;
    std::cout << std::endl;
    std::cout << "Volume mesh formats:" << std::endl;
    std::cout << "  VTK Legacy (.vtk)" << std::endl;
    std::cout << "  VTK XML (.vtu/.vtp/.vti/.vts)" << std::endl;
    std::cout << "  CGNS (.cgns)" << std::endl;
    std::cout << "  Gmsh v2/v4 (.msh)" << std::endl;
    std::cout << "  SU2 (.su2)" << std::endl;
    std::cout << "  OpenFOAM (foamFile)" << std::endl;
    std::cout << std::endl;
    std::cout << "Surface mesh formats:" << std::endl;
    std::cout << "  STL ASCII/Binary (.stl)" << std::endl;
    std::cout << "  OBJ (.obj)" << std::endl;
    std::cout << "  PLY ASCII/Binary (.ply)" << std::endl;
    std::cout << "  OFF (.off)" << std::endl;
}

MeshFormat stringToFormat(const std::string& formatStr) {
    std::string lowerStr = formatStr;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    
    if (lowerStr == "stl") return MeshFormat::STL_ASCII;
    if (lowerStr == "obj") return MeshFormat::OBJ;
    if (lowerStr == "ply") return MeshFormat::PLY_ASCII;
    if (lowerStr == "off") return MeshFormat::OFF;
    if (lowerStr == "vtk") return MeshFormat::VTK_LEGACY;
    if (lowerStr == "vtu") return MeshFormat::VTK_XML;
    if (lowerStr == "cgns") return MeshFormat::CGNS;
    if (lowerStr == "msh") return MeshFormat::GMSH_V4;
    if (lowerStr == "su2") return MeshFormat::SU2;
    
    return MeshFormat::UNKNOWN;
}

bool parseCommandLine(int argc, char* argv[], CommandLineOptions& options) {
    int i = 1;
    while (i < argc) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            options.help = true;
            i++;
        } else if (arg == "-v" || arg == "--version") {
            options.version = true;
            i++;
        } else if (arg == "-l" || arg == "--list-formats") {
            options.listFormats = true;
            i++;
        } else if (arg == "-s" || arg == "--source-format") {
            if (i + 1 < argc) {
                options.sourceFormat = argv[i + 1];
                i += 2;
            } else {
                return false;
            }
        } else if (arg == "-t" || arg == "--target-format") {
            if (i + 1 < argc) {
                options.targetFormat = argv[i + 1];
                i += 2;
            } else {
                return false;
            }
        } else if (arg == "-V" || arg == "--verbose") {
            options.verbose = true;
            i++;
        } else if (arg == "--no-cleaning") {
            options.processingOptions.enableCleaning = false;
            i++;
        } else if (arg == "--triangulate") {
            options.processingOptions.enableTriangulation = true;
            i++;
        } else if (arg == "--decimate") {
            if (i + 1 < argc) {
                options.processingOptions.enableDecimation = true;
                options.processingOptions.decimationTarget = std::stod(argv[i + 1]);
                i += 2;
            } else {
                return false;
            }
        } else if (arg == "--smooth") {
            if (i + 1 < argc) {
                options.processingOptions.enableSmoothing = true;
                options.processingOptions.smoothingIterations = std::stoi(argv[i + 1]);
                i += 2;
            } else {
                return false;
            }
        } else if (arg == "--compute-normals") {
            options.processingOptions.enableNormalComputation = true;
            i++;
        } else if (arg[0] != '-') {
            if (options.inputFile.empty()) {
                options.inputFile = arg;
                i++;
            } else if (options.outputFile.empty()) {
                options.outputFile = arg;
                i++;
            } else {
                return false;
            }
        } else {
            return false;
        }
    }
    
    return true;
}

int main(int argc, char* argv[]) {
    CommandLineOptions options;
    
    if (!parseCommandLine(argc, argv, options)) {
        printHelp();
        return 1;
    }
    
    if (options.help) {
        printHelp();
        return 0;
    }
    
    if (options.version) {
        printVersion();
        return 0;
    }
    
    if (options.listFormats) {
        printSupportedFormats();
        return 0;
    }
    
    if (options.inputFile.empty() || options.outputFile.empty()) {
        std::cerr << "Error: Please specify input and output files" << std::endl;
        printHelp();
        return 1;
    }
    
    MeshFormat sourceFormat = MeshFormat::UNKNOWN;
    MeshFormat targetFormat = MeshFormat::UNKNOWN;
    
    if (!options.sourceFormat.empty()) {
        sourceFormat = stringToFormat(options.sourceFormat);
        if (sourceFormat == MeshFormat::UNKNOWN) {
            std::cerr << "Error: Unsupported source format: " << options.sourceFormat << std::endl;
            std::cerr << std::endl;
            std::cerr << "Supported formats:" << std::endl;
            std::vector<std::string> formatNames = MeshHelper::getSupportedFormatNames();
            for (const auto& name : formatNames) {
                std::cerr << "  - " << name << std::endl;
            }
            return 1;
        }
    } else {
        sourceFormat = MeshHelper::detectFormat(options.inputFile);
        if (sourceFormat == MeshFormat::UNKNOWN) {
            std::cerr << "Error: Cannot detect input file format for \"" << options.inputFile << "\"" << std::endl;
            std::cerr << std::endl;
            std::cerr << "Possible reasons:" << std::endl;
            std::cerr << "  1. File does not exist" << std::endl;
            std::cerr << "  2. Unsupported file extension" << std::endl;
            std::cerr << "  3. Invalid or corrupted file" << std::endl;
            std::cerr << std::endl;
            std::cerr << "Solutions:" << std::endl;
            std::cerr << "  1. Verify the input file path is correct" << std::endl;
            std::cerr << "  2. Use one of the supported file extensions" << std::endl;
            std::cerr << "  3. Or specify the source format explicitly using --source-format option" << std::endl;
            std::cerr << "     Example: --source-format vtu" << std::endl;
            std::cerr << std::endl;
            std::cerr << "Supported formats:" << std::endl;
            std::vector<std::string> formatNames = MeshHelper::getSupportedFormatNames();
            for (const auto& name : formatNames) {
                std::cerr << "  - " << name << std::endl;
            }
            return 1;
        }
    }
    
    if (!options.targetFormat.empty()) {
        targetFormat = stringToFormat(options.targetFormat);
        if (targetFormat == MeshFormat::UNKNOWN) {
            std::cerr << "Error: Unsupported target format: " << options.targetFormat << std::endl;
            std::cerr << std::endl;
            std::cerr << "Supported formats:" << std::endl;
            std::vector<std::string> formatNames = MeshHelper::getSupportedFormatNames();
            for (const auto& name : formatNames) {
                std::cerr << "  - " << name << std::endl;
            }
            return 1;
        }
    } else {
        std::string errorMsg;
        if (!MeshHelper::validateOutputFormat(options.outputFile, errorMsg)) {
            std::cerr << errorMsg << std::endl;
            return 1;
        }
        targetFormat = MeshHelper::detectFormatFromExtension(options.outputFile);
    }
    
    if (options.verbose) {
        std::cout << "Input file: " << options.inputFile << std::endl;
        std::cout << "Output file: " << options.outputFile << std::endl;
        std::cout << "Source format: " << (options.sourceFormat.empty() ? "Auto-detected" : options.sourceFormat) << std::endl;
        std::cout << "Target format: " << (options.targetFormat.empty() ? "Auto-detected" : options.targetFormat) << std::endl;
        std::cout << "Processing options: " << std::endl;
        std::cout << "  - Point cleaning: " << (options.processingOptions.enableCleaning ? "Enabled" : "Disabled") << std::endl;
        std::cout << "  - Triangulation: " << (options.processingOptions.enableTriangulation ? "Enabled" : "Disabled") << std::endl;
        std::cout << "  - Mesh decimation: " << (options.processingOptions.enableDecimation ? "Enabled (" + std::to_string(options.processingOptions.decimationTarget) + ")" : "Disabled") << std::endl;
        std::cout << "  - Mesh smoothing: " << (options.processingOptions.enableSmoothing ? "Enabled (" + std::to_string(options.processingOptions.smoothingIterations) + " iterations)" : "Disabled") << std::endl;
        std::cout << "  - Normal computation: " << (options.processingOptions.enableNormalComputation ? "Enabled" : "Disabled") << std::endl;
    }
    
    FormatWriteOptions writeOptions;
    MeshErrorCode errorCode;
    std::string errorMsg;
    
    bool success = VTKConverter::convert(
        options.inputFile,
        options.outputFile,
        sourceFormat,
        targetFormat,
        options.processingOptions,
        writeOptions,
        errorCode,
        errorMsg
    );
    
    if (success) {
        std::cout << "Conversion successful!" << std::endl;
        if (options.verbose) {
            std::cout << "Input file: " << options.inputFile << std::endl;
            std::cout << "Output file: " << options.outputFile << std::endl;
            std::cout << "Conversion completed using VTK as intermediate format" << std::endl;
        }
    } else {
        std::cerr << "Conversion failed: " << errorMsg << " (Error code: " << static_cast<int>(errorCode) << ")" << std::endl;
        return 1;
    }
    
    return 0;
}
