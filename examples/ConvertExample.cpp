#include <iostream>
#include "MeshConverter.h"
#include "MeshWriter.h"
#include "MeshHelper.h"

/**
 * @brief Single file conversion example (VTK→CGNS)
 */
int main() {
    // 1. Define parameters
    std::string srcPath = "input.vtk";
    std::string dstPath = "output.cgns";
    MeshFormat srcFormat = MeshHelper::detectFormat(srcPath);
    MeshFormat dstFormat = MeshFormat::CGNS;
    FormatWriteOptions writeOptions;
    
    // Configure CGNS-specific options
    writeOptions.cgnsBaseName = "CFD_Base";
    writeOptions.cgnsZoneName = "Flow_Zone";
    writeOptions.cgnsDimension = 3;
    writeOptions.isBinary = true;

    // 2. Execute conversion
    MeshErrorCode errorCode;
    std::string errorMsg;
    bool success = MeshConverter::convert(srcPath, dstPath, srcFormat, dstFormat, writeOptions, errorCode, errorMsg);

    // 3. Check result
    if (success) {
        std::cout << "Conversion successful!" << std::endl;
    } else {
        std::cerr << "Conversion failed: " << errorMsg << " (Error code: " << static_cast<int>(errorCode) << ")" << std::endl;
    }

    return success ? 0 : 1;
}
