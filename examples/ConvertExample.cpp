#include <iostream>
#include "MeshConverter.h"
#include "MeshWriter.h"
#include "MeshHelper.h"

/**
 * @brief 单文件转换示例（VTK→CGNS）
 */
int main() {
    // 1. 定义参数
    std::string srcPath = "input.vtk";
    std::string dstPath = "output.cgns";
    MeshFormat srcFormat = MeshHelper::detectFormat(srcPath);
    MeshFormat dstFormat = MeshFormat::CGNS;
    FormatWriteOptions writeOptions;
    
    // 配置CGNS专属选项
    writeOptions.cgnsBaseName = "CFD_Base";
    writeOptions.cgnsZoneName = "Flow_Zone";
    writeOptions.cgnsDimension = 3;
    writeOptions.isBinary = true;

    // 2. 执行转换
    MeshErrorCode errorCode;
    std::string errorMsg;
    bool success = MeshConverter::convert(srcPath, dstPath, srcFormat, dstFormat, writeOptions, errorCode, errorMsg);

    // 3. 结果判断
    if (success) {
        std::cout << "转换成功！" << std::endl;
    } else {
        std::cerr << "转换失败：" << errorMsg << "（错误码：" << static_cast<int>(errorCode) << "）" << std::endl;
    }

    return success ? 0 : 1;
}
